////////////////////////////////////////////
// COMMAND LINE and ENV PROCESSING
// 
//   scgi [ --conf file ]
//
//     1. special case for no command line options/arguments.
//        - if present, it must be the only option
//        - it is implicitly present, if the command line is empty
//        - if it is implicitly present, the config file is only 
//          source if it exists -- no error otherwise
//
//   scgi [options] ( --port=<number> | --socket )
//
//      2. normal processing of the command line for options
//
//   scgi [--version | --help ] [ ... ]
//
//      3. special case with use of --version and --help
//         - regardless of the composition of the command line
//         - if either --version or --help is present:
//
//

#include "general.h"

/* COMMAND LINE OPTIONS */
#define  o_BIND       (1)
#define  o_CONF       (2)
#define  o_FILTER     (4)
#define  o_FOREGROUND (5)
#define  o_DAEMON    (-5)
#define  o_HELP       (6)
#define  o_PORT       (8)
#define  o_SOCKET     (9)
#define  o_UDS       (10)
#define  o_USER      (11)
#define  o_VERBOSE   (12)
#define  o_VERSION   (13)
#define  o_WORKERS   (14)
#define  e_SCGI_DIR  (15)

#define  MAX_OPTIONS 20


// The following are variables that are defined via command line processing.
extern char * server_ip;       // o_BIND
extern int    port;            // o_PORT
extern char * filter_name;     // o_FILTER
extern int    socket_flag;     // o_SOCKET
extern int    user;            // o_USER
extern int    foreground_flag; // o_FOREGROUND
extern int    verbose_level;   // o_VERBOSE
extern int    workers;         // o_WORKERS
extern int    help_flag;       // o_HELP
extern int    version_flag;    // o_VERSION
extern char * scgi_dir;        // o_SCGI_DIR


extern void set_command_line_options(int argc, char *argv[]);
