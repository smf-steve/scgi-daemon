#include "scgi_netstring.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

#define SUCCESS (0)


// Values associated with the Header, provided for readability
#define CONTENT_LENGTH "CONTENT_LENGTH"
#define SCGI_NAME "SCGI"
#define SCGI_VALUE '1'

#define CGI_NAME  "GATEWAY_INTERFACE"
#define CGI_VALUE "1.1"

#define MAX_ENV_COUNT ( NETSTRING_MAX_STRINGS_DEFAULT / 2 )


static const char * error_msg[] = {
  "Netstring: SUCCESS",
  "Netstring Error: INVALID_SIZE (1)",
  "Netstring Error: MISSING_COLON (2)",
  "Netstring Error: TRUNCATED_STRING (3)",
  "Netstring Error: MISSING_TRAILING_COMMA (4)",
  "SCGI Error: PROTOCOL_ERROR (5)",
  "SCGI Error: MISSING_CONTENT_LENGTH (6)",
  "SCGI Error: EMPTY_CONTENT_LENGTH (7)",
  "SCGI Error: INVALID_VERSION (8)",
  "SCGI Error: DUPLICATE_ENVS (9)",
  "SCGI Error: TOO_MANY_ENVS (10)",
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

  return_error( (count < 4),     SCGI_PROTOCOL_ERROR);
  return_error( (count % 2 != 0), SCGI_PROTOCOL_ERROR);

  return_error((strcmp(strings[0], CONTENT_LENGTH) !=0),
                SCGI_MISSING_CONTENT_LENGTH);

  return_error((strcmp(strings[1], "") == 0),
                SCGI_EMPTY_CONTENT_LENGTH);

  int scgi_version = 0;
  for(int i=2; i < count; i+=2) {
    if (! strcmp(strings[i], SCGI_NAME)) {
      char *value = strings[i+1];

      scgi_version = value[0] + value[1];
      break;
    }
  }
  return_error((scgi_version != SCGI_VALUE), SCGI_INVALID_VERSION);


  // Technically, the SCGI protocol states that
  // there should be no duplicate names.
  // In this implementation, we do not validate
  // this requirement.                           


  return SUCCESS;
}


char ** scgi_netstring2env(NETSTRING *ns_p) {

  char **strings  =  ns_p->strings;
  int  count      =  ns_p->strings_count;

  if (count % 2 != 0) {
    fprintf(stderr, "%s\n", error_msg[SCGI_PROTOCOL_ERROR]);
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


/* A simple macro used to walk the pointer p through the buffer, */
/* looking for the char immediately following the next NULL char */
#define next_start(p) { while ( *p != '\0' ) p++; p++; }

/* A macro to convert 2 consecutive null-terminate strings into  */
/* a single string containing and env definition:                */
#define append_env_value(_name,_value) (*(_value-1) = '=', _name)

// user is responsible for freeing the env value
extern int scgi_read2env(int fd, char *env[], int env_size) {

  int  retval = 0;
  char preamble_buffer[NETSTRING_PREAMBLE_MAX];
  char *next_p;
  int  header_size = 0;
  int comma = '\0';

  char *buffer;         /* A buffer for the <header>                                        */
  int  to_read;
  int  read_chars;
  int  residual;

  int scgi_version = 0;

  read(STDIN_FILENO, preamble_buffer, NETSTRING_PREAMBLE_MAX);
  *(preamble_buffer + NETSTRING_PREAMBLE_MAX) = '\0';

  header_size = (size_t) strtol(preamble_buffer, &next_p, 10);
     return_error( (*next_p != ':'), NETSTRING_MISSING_COLON);

  buffer = malloc(sizeof(char) * header_size);
  residual =  NETSTRING_PREAMBLE_MAX - (next_p - preamble_buffer + 1);

  strncpy(buffer, next_p+1, residual);

  to_read = header_size - residual + 1;
  read_chars = read(STDIN_FILENO, buffer+residual, to_read);
  comma = *(buffer + header_size + 1);
    return_error((read_chars != to_read), NETSTRING_TRUNCATED_STRING);
    return_error((comma != ','), NETSTRING_MISSING_TRAILING_COMMA);

  /* PROCESS THE HEADER and create the ENV */
  {
    int env_count = 0;
    char * p     = buffer; /* Walking pointer */
    char *_name, *_value;  /* Marker pointers */

    _name  = p; next_start(p);
    _value = p; next_start(p);

    /* Read the required CONTENT_LENGTH <header> line */
    retval = strcmp(_name, CONTENT_LENGTH);
      return_error((retval != 0), SCGI_MISSING_CONTENT_LENGTH);
      return_error((*_value == '\0'), SCGI_EMPTY_CONTENT_LENGTH);

    env[env_count] = append_env_value(_name, _value);
    env_count ++;

    /* Process one or more <header> lines */
    while (p < (buffer + header_size)) {
      _name  = p; next_start(p);
      _value = p; next_start(p);

      env[env_count] = append_env_value(_name, _value);
      env_count ++;
      assert(env_count < env_size);

      /* Per the Protocol, check for the SCGI_NAME */
      if (! strcmp(_name, SCGI_NAME)) {
        /* SCGI_VALUE must be "1" */
        /* hence _value[0] == '1' && _value[1] == '\0' */
        scgi_version = _value[0] + _value[1];
        break;
      }
    }
    return_error((scgi_version != SCGI_VALUE), SCGI_INVALID_VERSION);

    /* Process remaining <header> lines */
    while (p < (buffer + header_size)) {
      _name  = p; next_start(p);
      _value = p; next_start(p);

      env[env_count] = append_env_value(_name, _value);
      env_count ++;
      assert(env_count < env_size);
    }

    env[env_count] = NULL;
      return_error((env_count >= env_size), SCGI_TOO_MANY_ENVS);
  }
  return SUCCESS;
}


/*

  count = fscanf(stdin, "%zu:" &header_size);read(STDIN_FILENO, preamble_buffer, NETSTRING_PREAMBLE_MAX);
     return_error( (count != 1), NETSTRING_MISSING_COLON);

  buffer = malloc(sizeof(char) * header_size);
  to_read = header_size - residual + 1;
  fread(buffer, sizeof(char), header_size + 1, stdin);
  comma = *(buffer + header_size + 1);
    return_error((read_chars != header_size), NETSTRING_TRUNCATED_STRING);
    return_error((comma != ','), NETSTRING_MISSING_TRAILING_COMMA);
*/
