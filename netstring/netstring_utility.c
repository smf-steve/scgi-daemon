/* File: netstring_utility.c                                   */
/*                                                             */
/* Purpose:                                                    */
/*   - To create a command line utility that facilates         */
/*     the encoding and decoding of a netstring.               */
/*   - To utilize said tool to perform system testing.         */
/*   - Doubles as the 'scgi_netstring' utility when            */
/*     when compiled with `-DSCGI_ENCODING                     */
/*                                                             */
/* Usage:                                                      */
/*   netstring [-e | --encode]  < env_file  > netstring        */
/*   netstring [-d | --decode]  < netstring > env_file         */
/*                                                             */
/*   scgi_ netstring [-e | --encode]  < env_file  > netstring  */
/*   scgi_ netstring [-d | --decode]  < netstring > env_file   */
/*                                                             */
/* Status:                                                     */
/*   - sufficient implementation to debug portions of the      */
/*     netstring implementating (contained within netstring.c) */
/*                                                             */
/* To Do:                                                      */
/*   - flesh out command line arguements                       */
/*   - establish STH test cases for testing                    */
/*   - integrate into the `scgi-launch` system                 */
/*                                                             */

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TRUE  (0)
#define FALSE (!TRUE)

#ifndef SCGI_ENCODING
#  include "netstring.h"
#else
#  include "scgi_netstring.h"
#endif

#define NETSTRING_ENCODE (0)
#define NETSTRING_DECODE (1)
#define NETSTRING_ERROR (3)


int main(int argc, char *argv[], char **envp) {

  int operation = NETSTRING_ENCODE;

  if (argc > 1) {
     operation = strcmp(argv[1], "-e")?         NETSTRING_ENCODE
               : strcmp(argv[1], "--encode")?   NETSTRING_ENCODE
               : strcmp(argv[1], "-d")?         NETSTRING_DECODE
               : strcmp(argv[1], "--decode")?   NETSTRING_DECODE
               : NETSTRING_ERROR;
  }
  
  switch (operation) {
    case NETSTRING_ENCODE:
      {
        NETSTRING *netstring_p;
        char      *str;
        size_t    length;

        netstring_p = netstring_start(0,0);

        str = fgetln(stdin, &length);
        while (length != 0) {
          // remove the '\n' deliminator
          if (str[length - 1] == '\n') {
             length --;
          } 
 
          netstring_append(netstring_p, str, length);
          str = fgetln(stdin, &length);
        }

        netstring_end(netstring_p);
        netstring_fwrite(netstring_p, stdout);
        netstring_free(netstring_p);
        break;
      }

    case NETSTRING_DECODE:
      { 
        NETSTRING *netstring_p;
        char      **env_p;

        netstring_p = netstring_allocate(0,0);
        netstring_read(STDIN_FILENO, netstring_p);        // Using layer 2 for ead, but layer 3 above.. need to implement fread
        netstring_end(netstring_p);

        // env_p = netstring_strings(netstring_p);
        env_p = netstring_p->strings;          // need an netstring access method

#ifdef SCGI_ENCODING
        scgi_netstring_validate(netstring_p);  // ensures the netstring conforms to the scgi protocol
        env_p = scgi_netstring2env(env_p);     // collapse (name, value) pairs into name=value strings 
#endif

        for(int count=0; env_p[count] != NULL; count++) {
           fprintf(stdout, "%s\n", env_p[count]);
        }

        netstring_free(netstring_p);
        break;

      default:
        assert(TRUE);
        break;
    }
  }

  return 0;

}
