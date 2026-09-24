#include "netstring.h"

// The Following Definitions exted the definitions in "netstring.h"
#define SCGI_PROTOCOL_ERROR (5)
#define SCGI_MISSING_CONTENT_LENGTH (6)
#define SCGI_EMPTY_CONTENT_LENGTH (7)
#define SCGI_INVALID_VERSION (8)
#define SCGI_DUPLICATE_ENVS (9)
#define SCGI_TOO_MANY_ENVS (10)

extern int scgi_netstring_validate(NETSTRING *ns_p);
  // Review contents of the netstring to
  // validate that it conforms to the scgi protocol

extern char ** scgi_netstring2env(NETSTRING *ns_p);
  // Concatenates assocociated name and value strings into
  // a single string separated by an '='
  // Resulting ns_p->strings[*] conforms to 'execle'


// The scgi_fread2env and scgi_read2env subroutines are a
// specialized implementation of the following general code:
//
// {
//   netstring_alloc();
//   netstring_fread();
//   scgi_netstring_validate();
//   scgi_netstring2env();
// }
//
// It is envisioned that these to subroutines would be used
// as part of a child process of a scgi server.
//

extern int scgi_fread2env(char *env[], int size, FILE * fp);
  // Intended to be used as part of a SCGI server
  // Reads a netstring from a Stream, creates an environ
  // containing the SCGI variables to be passed
  // directly to `execle`

extern int scgi_read2env(int fd, char *env[], int size);
  // Intended to be used as part of a SCGI server
  // Reads a netstring from a file, creates an environ
  // containing the SCGI variables to be passed
  // directly to `execle`


#define scgi_free_env_buffer(env)  free(env[0])

