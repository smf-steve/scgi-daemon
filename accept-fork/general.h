#define SET (1)
#define UNSET (0)

#define PID_FILE      "pid"
#define ERROR_FILE    "error.log"
#define DOCUMENT_DIR  "cgi"
#define SOCKET_FILE   "socket"

#define MAX_ENVS (50)

// exit codes
// 0: successful
// 1: standard: command line option errors (stemming from command_line)
// 2: runtime: errors within scgi_server with scgi execution chain, e.g., exec error
// 3: panics: errors that should NOT occurr 

#define panic(str) fprintf(stderr, "panic at %s:%s, str\n", str, __FILE__ , __LINE__); exit()

