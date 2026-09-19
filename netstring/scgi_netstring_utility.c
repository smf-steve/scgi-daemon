/*****************************************************************/
/* File: netstring_utility.c                                     */
/*                                                               */
/* Purpose:                                                      */
/*   - To create a command line utility that facilates           */
/*     the encoding and decoding of a netstring.                 */
/*   - Doubles as the 'scgi_netstring' utility when              */
/*     when compiled with `-DSCGI_ENCODING`                      */
/*   - To utilize said tool to perform system testing.           */
/*                                                               */
/* Usage:                                                        */
/*   netstring [-e] [options] < env_file  > netstring            */
/*   netstring [-d] [options] < netstring > env_file             */
/*                                                               */
/*   scgi_netstring [-e] [options]  < env_file  > netstring      */
/*   scgi_netstring [-d] [options]  < netstring > env_file       */
/*                                                               */
/* Options:                                                      */
/*    -e, --encode    : encode a sequence of text strings ('\n'  */
/*                      terminated) into a netstring. DEFAULT    */
/*                                                               */
/*    -d, --decode    : decode a netstring into a sequence of    */
/*                      text strings ('\n' terminated)           */
/*                                                               */
/*    --min-length=n  : set the mininum length of the sequence   */
/*                      of text strings.                         */
/*                      (default: --min-length=NS_MIN_LENGTH     */
/*                                                               */
/*    --max-length=n  : set the maximum length of the sequence   */
/*                      of text strings.                         */
/*                      (default value: see below)               */
/*                                                               */
/*    --max-strings=n : set the maxium number of text strings    */
/*                      encoded/decode                           */
/*                      (default value: see below)               */
/*                                                               */
/*    --use-file      : use file descriptors (int fd) for I/O    */
/*                      DEFAULT                                  */
/*                                                               */
/*    --use-stream    : use streams (FILE *fp) for I/O           */
/*                                                               */
/* Environment Variables:                                        */
/*    In liu of the above command-line options, environment      */
/*    variables can be defined to achieve the same effect. If    */
/*    hese variablers are not defined, the default value are:    */  
/*                                                               */
/*    NETSTRING_MIN_LENGTH  : 0                                  */
/*    NETSTRING_MAX_LENGTH  : 65,535  (0xFFFF)                   */
/*    NETSTRING_MAX_STRINGS : 255                                */
/*    NETSTRING_USE_FILES   : defined                            */
/*    NETSTRING_USE_STREAMS : undefined (overrides USE_FILES)    */
/*                                                               */
/*****************************************************************/

#ifndef SCGI_ENCODING
#  include "netstring.h"
#else
#  include "scgi_netstring.h"
#endif

#define TRUE  (0)
#define FALSE (!TRUE)

#define NETSTRING_ENCODE (0)
#define NETSTRING_DECODE (1)
#define NETSTRING_ERROR  (3)

#define NETSTRING_USE_FDS     (0)
#define NETSTRING_USE_STREAMS (1)
// By default program uses file descriptors


int main(int argc, char *argv[], char **envp) {

  // DEFAULT operations of the program
  int operation    = NETSTRING_ENCODE;
  int io_mechanism = NETSTRING_USE_STREAMS;

  if (argc > 1) {
    if (strcmp(argv[1], "-e") == 0 ) {
       operation = NETSTRING_ENCODE;
     } else if (strcmp(argv[1], "-d") == 0 ) {
       operation = NETSTRING_DECODE;
     } else {
       operation = NETSTRING_ERROR;
     }
   }
  
  switch (operation) {
    case NETSTRING_ENCODE:
      {
        NETSTRING *ns_p;
        char      *str;
        size_t    length;

        ns_p = netstring_start(0,0);

        str = fgetln(stdin, &length);
        while (length != 0) {

          // remove the '\n' deliminator
          if (str[length - 1] == '\n') {
             str[length - 1] = '\0';
             length --;
          } 
 
          netstring_append(ns_p, str, length);
          str = fgetln(stdin, &length);
        }

        netstring_end(ns_p);

        if (io_mechanism == NETSTRING_USE_STREAMS) {
          netstring_fwrite(ns_p, stdout); // Using layer 3 for I/O
        } else {
          netstring_write(STDOUT_FILENO, ns_p);  // Using layer 2 for I/O
        }

        netstring_free(ns_p);
        break;
      }

    case NETSTRING_DECODE:
      { 
        NETSTRING *ns_p;
        char      **strings;
        int       ret_val;

        ns_p = netstring_allocate(0,0);

        if (io_mechanism == NETSTRING_USE_STREAMS) {
          netstring_fread(ns_p, stdin);        // Using layer 3 for I/O
        } else {
          netstring_read(STDIN_FILENO, ns_p);  // Using layer 2 for I/O
        }
        netstring_end(ns_p);

        // env_p = netstring_strings(ns_p);
        strings = ns_p->strings;          // need an netstring access method
        //netstring_get_strings(ns_p);

#ifdef SCGI_ENCODING
        ret_val = scgi_netstring_validate(ns_p);  // ensures the netstring conforms to the scgi protocol
        if (ret_val != 0) {
          exit(ret_val);
        }

        strings = scgi_netstring2env(ns_p);     // collapse (name, value) pairs into name=value strings 
#endif

        for(int count=0; strings[count] != NULL; count++) {
           fprintf(stdout, "%s\n", strings[count]);
        }

        netstring_free(ns_p);
        break;

      default:
        assert(TRUE);
        break;
    }
  }

  return 0;

}
