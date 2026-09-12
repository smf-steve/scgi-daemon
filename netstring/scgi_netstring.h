#include "netstring.h"

extern int scgi_netstring_validate(NETSTRING *ns_p);
  // Review contents of the netstring to
  // validate that it conforms to the scgi protocol

extern char ** scgi_netstring2env(NETSTRING *ns_p);
  // Concatenates assocociated name and value strings into
  // a single string separated by an '='
  // Resulting ns_p->strings[*] conforms to 'execle'
