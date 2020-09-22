#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PROGRAM (argv[1])

#define CONTENT_LENGTH "CONTENT_LENGTH"
#define SCGI_NAME "SCGI"
#define SCGI_VALUE "1"
#define MAX_BUFFER (1024)

#define NONZERO (!0)

#define next_start(p) { while ( *p != '\0' ) p++; p++; }

int main(int argc, char * argv[]) {
       int request_size = 0;
       int body_size = 0;

       char buffer[MAX_BUFFER];
       char *p;
       
       char *_name;
       char *_value;

       
       scanf("%d:", &request_size);
       if (request_size > MAX_BUFFER ) {
	 fprintf(stderr, "MAX_BUFFER size exceeded");
	 exit(1);
       }

       
       /* Read the Header based upon the size */
       fread(&buffer, 1, request_size + 1, stdin);

       p = buffer;

       /* The first header must for the CONTENT_LENGTH */
       sscanf(p, CONTENT_LENGTH);  next_start(p);
       setenv(CONTENT_LENGTH, p, NONZERO); 

       body_size = atoi(p);
       next_start(p);
       
       while (p < (buffer + request_size)) {
	 _name = p; next_start(p);
	 _value = p; next_start(p)
	 setenv(_name, _value, NONZERO);

	 /* Per the Protocol, check for the SCGI_NAME */
	 if (! strcmp(_name, SCGI_NAME)) {
	   if ( strcmp(_value, SCGI_VALUE)) {
		 fprintf(stderr, "SCGI_VALUE is unsupported\n");
		 exit(1);
	       }
	 }

	 /* Per the Protocol, each _name processed should be unique. We presume this is true. */
	 
       }

       /* The Body of the request is left on stdio */
       /* Exec the next program */

       execl(PROGRAM, PROGRAM, (char *) NULL);
       exit(1);
}
