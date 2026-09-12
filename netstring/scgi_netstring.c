#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scgi_netstring.h"

/* File: scgi_netstrings.c                                   */
/*                                                           */
/* Purpose:                                                  */
/*   Given an a netstring                                    */
/*   1. determine if the netstring is scgi-compliant         */
/*   2. extract the CGI environment variables                */
/*   3. allow the said environment variables to be           */
/*      used in a call to 'execle'                           */
/*   4. Appends the CGI environment variable to the          */
/*      netstring                                            */
/*                                                           */
/* Description:                                              */
/*   The given netstring is a sequence of C-strings that     */
/*   represent the name and value for each CGI variable.     */
/*   Both the name and value are sent as individual strings. */
/*                                                           */
/*   Once the netstring is determined to be scgi-compliant,  */
/*   each name-value pair is transformed from two C-strings  */
/*   into a single C-strings of the form: <name>=<value>     */
/*                                                           */
/*   Moreover, the revised set of C-strings is addressable   */
/*   via a 'char *strings[]' thus                            */
/*                                                           */
/* Caveot:                                                   */
/*   This implementation of scgi-netstring relies on the     */
/*   the provided netstring implementation.  The NETSTRING   */
/*   datastructure contains a 'char *strings[]' that is      */
/*   maintained, and this structure is used by our           */
/*   implementation of the scgi-netstring                    */
/*                                                           */
/*********************************************************************/
/* SCGI Protocol Definition:  http://python.ca/scgi/protocol.txt     */ 
/*                                                                   */
/* Description:                                                      */
/*   The SCGI protocol utilizes a modified netstring to transmit     */
/*   the contents of the  of a netstring.                            */
/*                                                                   */
/* Syntax:                                                           */
/*     P         ->    <h_size> ":" <header> "," <body>              */
/*     <header>  ->    <name> '\0' <value> '\0'                      */
/*                   ( <name> '\0' <value> '\0' )+                   */
/*                                                                   */
/*     <h_size>   : the size in bytes for the <header>               */
/*     <b_size>   : the size in bytes for the <body>                 */
/*     <name>     : the name of an SCGI environment variable         */
/*     <value>    : the associated value of the SCGI variable        */
/*                                                                   */
/* Input Requirements:                                               */
/*     - There must be at least two name-value pairs                 */
/*     - The CONTENT_LENGTH variable must be the first header field  */
/*     - The value of the CONTENT_LENGTH variable must not be empty  */
/*     - SCGI name must exist                                        */
/*     - The value of the SCGI variable must have the value of '1'   */
/*     - There are no duplicate names                                */
/*                                                                   */
/* Assumptions:                                                      */
/*     - There are no duplicated names                               */
/*     - The maximum number of names is <MAX_ENV_COUNT>              */
/*                                                                                                                                   */


// Values associated with the Header, provided for readability   
#define CONTENT_LENGTH "CONTENT_LENGTH"
#define SCGI_NAME "SCGI"
#define SCGI_VALUE '1'

#define CGI_NAME  "GATEWAY_INTERFACE"
#define CGI_VALUE "1.1"

#define MAX_ENV_COUNT (NETSTRING_ARRAY_MAX/2)

// Error Values and Associated Error Messages                                                      
#define ERROR_SUCCESS (0)
#define ERROR_PROTOCOL_ERROR (1)
#define ERROR_MISSING_CONTENT_LENGTH (2)
#define ERROR_EMPTY_CONTENT_LENGTH (3)
#define ERROR_INVALID_SCGI_VERSION (4)
#define ERROR_DUPLICATE_ENVS (5)
#define ERROR_TOO_MANY_ENVS (6)
#define ERROR_OTHER (7)

static const char * error_msg[] = {
  "SCGI Error: SUCCESS (0)",
  "SCGI Error: PROTOCOL ERROR (1)",
  "SCGI Error: MISSING CONTENT_LENGTH (2)",
  "SCGI Error: EMPTY CONTENT_LENGTH (3)",
  "SCGI Error: INVALID SCGI VERSION (4)",
  "SCGI Error: DUPLICATE ENVS (5)",
  "SCGI Error: TOO MANY ENVS (6)",
  "SCGI Error: OTHER (7)"
};


// A simple macro used to test for an error, print the error,     
// and then return a NULL pointer                                 
// Used to make the code more readable                            
#define return_error(b,v) if (b) { \
      fprintf(stderr, "%s\n", error_msg[v]); return v; }


                       
int scgi_netstring_validate(NETSTRING *ns_p) {
  //  Ensure there is at least 4 strings
  //  Ensure the array has an even number of strings             
  //  Ensure the first _name is "CONTENT_LENGTH"                 
  //  Ensure the first _value is non-empty                       
  //  Ensure SCGI appears as a _name                             
  //  Ensure SCGI value is '1'    
  //  Ensure there are no duplicate _name's -- NOT implemented

  char **strings  =  ns_p->strings;
  int  count      =  ns_p->strings_count;

  return_error( (count < 4),     ERROR_PROTOCOL_ERROR);
  return_error( (count % 2 != 0), ERROR_PROTOCOL_ERROR);

  return_error((strcmp(strings[0], "CONTENT_LENGTH") !=0),
                ERROR_MISSING_CONTENT_LENGTH);

  return_error((strcmp(strings[1], "") == 0),
                ERROR_EMPTY_CONTENT_LENGTH);

  int scgi_version = 0;
  for(int i=2; i < count; i+=2) {
    if (! strcmp(strings[i], SCGI_NAME)) {
      char *value = strings[i+1];

      scgi_version = value[0] + value[1];    
      break;
    }
  }
  return_error((scgi_version != SCGI_VALUE), ERROR_INVALID_SCGI_VERSION);

  
  // Technically, the SCGI protocol states that  
  // there should be no duplicate names.
  // In this implementation, we do not validate  
  // this requirement.                           


  return ERROR_SUCCESS;
}


char ** scgi_netstring2env(NETSTRING *ns_p) {

  char **strings  =  ns_p->strings;
  int  count      =  ns_p->strings_count;

  
  if (count % 2 != 0) {
    fprintf(stderr, "%s\n", error_msg[ERROR_PROTOCOL_ERROR]);
    return NULL;
  }


  // Reduce the strings structure in half      
  //   1. replace '\0' before the value to be '='  
  //   2. collapse the strings structure       
  //   3. update the strings_count             
  for(int i=0; i < count; i +=2) {
    // name  = strings[i]
    // value = strings[i+1]
    *(strings[i+1] -1) = '=';
  }
  {
    count = count / 2;
    for(int i=1; i < count; i++) {
       strings[i] = strings[i*2];
    }
    strings[count] = NULL;
  }
  ns_p->strings_count = count;

  return ns_p->strings;
}

