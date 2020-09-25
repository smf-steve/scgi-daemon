/**********************************************************************/
/*  The scgi2env-exec programs:                                       */
/*     - reads an SCGI request,                                       */
/*     - prepares an environmment with CGI variables                  */
/*     - exec-s the program provided as its only arguement.           */
/*                                                                    */
/**********************************************************************/
/* SCGI Protocol Definition:  http://python.ca/scgi/protocol.txt      */
/*                                                                    */
/* Syntax:  P         ->  <h_size> ":" <header> "," <body>            */
/*          <header>  ->  "CONTENT_LENGTH" '\0' <b_size>              */
/*                         ( _name '\0' _value '\0' )+                */
/*          <h_size>   : the size in bytes for the <header>           */
/*          <b_size>   : the size in bytes for the <body>             */
/*                                                                    */
/* Input Requirements:                                                */
/*     - CONTENT_LENGTH must be the first header field                */
/*     - A header field (name/value) SCGI/1 must exist                */
/*     - There are no duplicate <header> "_name"s                     */
/* Output Requirements:                                               */
/*     - The response header must include a Status and Content-type:  */
/*     * e.g.,                                                        */
/*         Status: 200 okay                                           */
/*         Content-Type: text/plain                                   */
/*                                                                    */
/* Assumptions:                                                       */
/*     - all necessary env variables are provided in the header       */
/*     - The maximum number of env variables is <MAX_ENV_COUNT>       */
/*     - the 'Status' response, if ommited, is assumed to be '200'    */
/*     - the 'program' called should provide the Status header        */
/*     - the 'program' called should provide the Content-Type header  */
/*     - the 'program' called is responsible for reading the <body>   */
/*                                                                    */
/* Usage:  scgi2env-exec <PROGRAM>                                    */
/*                                                                    */
/* Return Values:                                                     */
#define RETVAL_SUCCESS (0)
#define RETVAL_PROTOCOL_ERROR (1)
#define RETVAL_UNABLE_TO_EXEC (2)
#define RETVAL_MISSING_CONTENT_LENGTH (3)
#define RETVAL_INVALID_SCGI (4)
#define RETVAL_TOO_MANY_ENVS (5)
#define RETVAL_OTHER (6)
/*                                                                    */
/**********************************************************************/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTE char
#define NONZERO (!0)

#define MAX_ENV_COUNT 100
#define PROGRAM (argv[1])

/* Values associated with the Header, provided for readiblity */
#define CONTENT_LENGTH "CONTENT_LENGTH"
#define SCGI_NAME "SCGI"
#define SCGI_VALUE '1'

/* A simple macro used to walk the pointer p thorugh the buffer, */
/* looking for the BYTE immediately following the next NULL char */
#define next_start(p) { while ( *p != '\0' ) p++; p++; }

/* A simple macro used to test for an error and then exit the    */
/* program used to make the code more readabable                 */
#define exit_error(b,v) if (b) exit(v); 

/* A simple macro that transforms the null char before _value to */
/* to an "=".   I.e., the string    "<_name> \0 <_value> \0"     */
/*            is tranformed into    "<_name> = <_value> \0"      */
#define append_env(n,v) (*(v-1) = '=', n)


int main(int argc, char * argv[], char **envp) {
  int header_size = 0;
  int body_size = 0;
  int retval = 0;
  char scgi_version;

  char * new_env[MAX_ENV_COUNT];
  
  BYTE * buffer;         /* A buffer for the <header>                                        */

  /* Determine the size of the header, and place the header and the "," into the buffer      */
  /*    -- leaving just the body on stdin.                                                   */
  retval = scanf("%d:", &header_size);              exit_error((retval != 1), RETVAL_PROTOCOL_ERROR);
  buffer = malloc(sizeof(BYTE) * (header_size+1));  exit_error((buffer == NULL), RETVAL_OTHER);
  retval = fread(buffer, 1, header_size+1, stdin);  exit_error((retval != header_size+1), RETVAL_OTHER);
  retval = buffer[header_size];                     exit_error((retval != ','), RETVAL_PROTOCOL_ERROR);


  /* PROCESS THE HEADER and create the ENV */
  {
    BYTE * p = buffer;     /* Use p as a walking pointer as we process the buffer.           */
    BYTE *_name, *_value;  /* Marker pointers for the start of the current _name and _value  */
    int env_count = 0;

    /* Read each of the header lines and place each into the Environment */
    {
      /* read the required CONTENT_LENGTH <header> line */
      _name  = p; next_start(p);
      _value = p; next_start(p);

      retval = strcmp(_name, CONTENT_LENGTH);       exit_error((retval != 0), RETVAL_MISSING_CONTENT_LENGTH);
      // body_size = atoi(_value);   The body_size is not used.

      new_env[env_count] = append_env(_name, _value);
      env_count ++;
      /* CONTENT_LENGTH <header> has been validated */


      /* Process one or more <header> lines */
      while (p < (buffer + header_size)) {
	_name  = p; next_start(p);
	_value = p; next_start(p);

	/* Per the Protocol, check for the SCGI_NAME */
	if (! strcmp(_name, SCGI_NAME)) {
	  /* value[1] should be '\0'. Hence, adding it to _value[0] has no impact */
	  scgi_version = _value[0] + _value[1];  
	}	 

	new_env[env_count] = append_env(_name, _value);
	env_count ++;

      }
      new_env[env_count] = NULL;

      exit_error((scgi_version != SCGI_VALUE), RETVAL_INVALID_SCGI);
      exit_error((env_count >= MAX_ENV_COUNT), RETVAL_TOO_MANY_ENVS);
    }
  }

  
  /* PROCESS THE BODY */
  {
    /* Exec the appropriate program to process the <body> and generate the response */
    
    /* Here we ASSUME that the called PROGRAM generates a valid response     */
    /* If we remove this assumption than the following steps should be taken */
    /*     1. for a child process                                            */
    /*     2. wire the child's stdin to the parents stdin                    */
    /*     3. reaad the results of the child (return value and output)       */
    /*     4. validate the assuptions, if not                                */
    /*        a. emit the appropriate header values                          */
    /*     5. write the childs output to stdout                              */
    
    execle(PROGRAM, PROGRAM, (char *) NULL, new_env);

    fprintf(stdout, "Status: 503 Service Unavailable\n");
    fprintf(stdout, "Content-type: text/plain\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Unsuccessful in execle %s\n", PROGRAM);
    fprintf(stdout, "\n");
    exit(RETVAL_UNABLE_TO_EXEC);
  }
}
