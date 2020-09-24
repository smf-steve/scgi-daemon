#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Protocol Definition:  http://python.ca/scgi/protocol.txt           */
/*                                                                    */
/* Syntax:  P         ->  <h_size> ":" <header> "," <body>            */
/*          <header>  ->  "CONTENT_LENGTH" '\0' <b_size>              */
/*                         ( _name '\0' _value '\0' )+                */
/*          <h_size>   : the size in bytes for the <header>           */
/*          <b_size>   : the size in bytes for the <body>             */
/*                                                                    */
/* Requirements:                                                      */
/*     - CONTENT_LENGTH must be the first header field                */
/*     - A header field (name/value) SCGI/1 must exist                */
/*     - There are no duplicate <header> "_name"s                     */
/*     - The response header must include a Status and Content-type:  */
/*     * e.g.,                                                        */
/*         Status: 200 okay                                           */
/*         Content-Type: text/plain                                   */
/*                                                                    */
/* Assumptions:                                                       */
/*     - all necessary env variables are provided in the header       */
/*     - maxium env variables to pass is MAX_ENV_COUNT                */
/*     - the 'Status' response is assumed to be '200' and emitted     */
/*     - the 'program' called must provide the Content-Type header    */
/*     - the 'program' called is responsible for reading the <body>   */
#define MAX_ENV_COUNT 100

/* Usage:  scgi2env-exec <PROGRAM>                                    */
/* Return Values:                                                     */
#define RETVAL_SUCCESS (0)
#define RETVAL_PROTOCOL_ERROR (1)
#define RETVAL_UNABLE_TO_EXEC (2)
#define RETVAL_MISSING_CONTENT_LENGTH (3)
#define RETVAL_INVALID_SCGI (4)
#define RETVAL_TOO_MANY_ENVS (5)
#define RETVAL_OTHER (6)


#define exit_error(b,v) if (b) exit(v); 


#define PROGRAM (argv[1])


/* Associated with the HEADER */
#define CONTENT_LENGTH "CONTENT_LENGTH"
#define SCGI_NAME "SCGI"
#define SCGI_VALUE '1'


#define BYTE char
#define NONZERO (!0)
#define next_start(p) { while ( *p != '\0' ) p++; p++; }

#define append_env(n,v) (*(v-1) = '=', n)

int main(int argc, char * argv[], char **envp) {
  int header_size = 0;
  int body_size = 0;
  int return_value = 0;
  char scgi_version;

  char * new_environment[MAX_ENV_COUNT];
  
  BYTE * buffer;         /* A buffer for the <header>                                        */

  /* Determine the size of the header, and place the header into the buffer, consume the "," */
  /*    -- leaving just the body on stdin.                                                   */
  return_value = scanf("%d:", &header_size);
  exit_error((return_value != 1), RETVAL_PROTOCOL_ERROR);
  
  buffer = malloc(sizeof(BYTE) * (header_size+1));
  exit_error((buffer == NULL), RETVAL_OTHER);
      
  return_value = fread(buffer, 1, header_size+1, stdin);
  exit_error((return_value != header_size+1), RETVAL_OTHER);
  
  return_value = buffer[header_size];
  exit_error((return_value != ','), RETVAL_PROTOCOL_ERROR);


  /* PROCESS THE HEADER and create the ENV */
  {
    BYTE * p = buffer;     /* Use p as a walking pointer as we process the buffer.           */
    BYTE *_name, *_value;  /* Marker pointers for the start of the current _name and _value  */
    int env_count = 0;

    /* Read each of the header lines and place each into the Environment */
    {
      /* read the required CONTENT_LENGTH <header> line */

      _name = p; next_start(p);
      _value = p; next_start(p);

      return_value = strcmp(_name, CONTENT_LENGTH);  
      exit_error((return_value != 0), RETVAL_MISSING_CONTENT_LENGTH);


      new_environment[env_count] = append_env(_name, _value);
      env_count ++;

      body_size = atoi(_value);


      /* Process one or more <header> lines */
      while (p < (buffer + header_size)) {
	_name = p; next_start(p);
	_value = p; next_start(p);

	/* Per the Protocol, check for the SCGI_NAME */
	if (! strcmp(_name, SCGI_NAME)) {
	  /* _value should be "1" */
	  scgi_version = _value[0] + _value[1];  
	}	 

	new_environment[env_count] = append_env(_name, _value);
	env_count ++;

      }
      new_environment[env_count] = NULL;

      exit_error((scgi_version != SCGI_VALUE), RETVAL_INVALID_SCGI);
      exit_error((env_count >= MAX_ENV_COUNT), RETVAL_TOO_MANY_ENVS);
    }
  }

  
  /* PROCESS THE BODY */
  /* Exec the appropriate program to process the <body> and generate the response */

  /* Test: emit the two assumed headers to see what will happen */
  fprintf(stdout, "Status: 200 Okay\n");

  /* A better way to handle this is to */
  /*    - fork a sub process  */
  /*    - wire the child's stdout to a part's file */
  /*    - obtain the return value of the child */
  /*    - generate the status return header line */
  /*    - validate there is a content-type, if not */
  /*       * emit default content-type */

  {
    char * my_env[] = {
		       "CONTENT_TYPE=tester",
		       "SCRIPT_FILENAME= hello this is it",
		       NULL};
    execle(PROGRAM, PROGRAM, (char *) NULL, new_environment);

    fprintf(stdout, "Status: 503 Service Unavailable\n");
    fprintf(stdout, "Content-type: text/plain\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Unsuccessful in execle %s\n", PROGRAM);
    fprintf(stdout, "\n");
    exit(RETVAL_UNABLE_TO_EXEC);
  }
}



/* program notes                     */
/* if [ "$SCGI" -eq "1" ] ; then     */
/*   echo "Status: 200 Okay"         */
/*   echo "Content-type: text/plain" */
/* fi                                */
/*                                   */
