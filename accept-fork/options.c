
#define NAMESPACE "SCGI_"
#define namespace(x) NAMESPACE x


/* COMMAND LINE OPTIONS */
#define  o_BIND       (1)
#define  o_CONF       (2)
#define  o_DOCUMENTS  (3)
#define  o_EXEC       (4)
#define  o_FOREGROUND (5)
#define  o_HELP       (6)
#define  o_LOG        (7)
#define  o_PORT       (8)
#define  o_SOCKET     (9)
#define  o_UDS        SO_ALIAS(o_SOCKET)
#define  o_USER      (10)
#define  o_VERBOSE   (11)
#define  o_VERSION   (12)
#define  o_WORKERS   (13)



static option_t options_list[] = {
  ///           index,        longName,     shortName, envName,             argType
  SETOPTION_ADD(o_BIND,       "bind",       'b',       "SCGI_SERVER_IP",
                OPT_ARG_REQUIRED);
  SETOPTION_ADD(o_CONF,       "config",     'd',       "",    
                OPT_ARG_REQUIRED | SO_ARG_ERROR_DUPLICATE);   
  SETOPTION_ADD(o_DOCUMENTS,  "documents",  'd',       "SCGI_DOCUMENTS",    
                OPT_ARG_REQUIRED);   
  SETOPTION_ADD(o_EXEC,       "exec",       'e',       "SCGI_EXEC",         OPT_ARG_REQUIRED);   
  SETOPTION_ADD(o_FOREGROUND, "foreground", 'f',       "SCGI_FOREGROUND",   OPT_ARG_NONE);       
  SETOPTION_ADD(o_LOG,        "log",        'l',       "SCGI_LOG",          OPT_ARG_REQUIRED);
  SETOPTION_ADD(o_PORT,       "port",       'p',       "SCGI_PORT",         OPT_ARG_REQUIRED);
  SETOPTION_ADD(o_SOCKET,     "socket",     's',       "SCGI_SOCKET_FILE",  OPT_ARG_REQUIRED);
  SETOPTION_ADD(o_UDS,        "uds",         0 ,       NULL,                0);
  SETOPTION_ADD(o_EXEC,       "exec",       'e',       "SCGI_EXEC",         OPT_ARG_REQUIRED);
  SETOPTION_ADD(o_WORKERS,    "workers",    'w',       "SCGI_WORKERS",      OPT_ARG_REQUIRED);
  SETOPTION_ADD(o_USER,       "user",       'u',       "SCGI_USER",         OPT_ARG_NONE);
  SETOPTION_ADD(o_VERBOSE,    "verbose",    'v',       "SCGI_VERBOSE",      OPT_ARG_NONE);
  SETOPTION_ADD(o_VERSION,    "version",    'V',       "SCGI_VERSION",      OPT_ARG_NONE);


  SETOPTION_ADD_END();
}


/* Alternatively you could

option_t * handle;

handle = 
  setoption_add(O_BIND, "bind", 'b', "SCGI_BIND", ARGTYPE);
  setoption_options(handle, initVal, defaultVal, duplicative);
  setoption_options(handle, initVal, defaultVal, duplicative);
  setoption_description(handle, "description", "<arg description>");

*/

/* we might also want to define
a variable to what we want

#define o_CONF_ARG (OPT_ARG_REQUIRED | SO_ARG_ERROR_DUPLICATE)

OR

setoption_arg(O_BIND, OPT_ARG_REQUIRED | SO_ARG_ERROR_DUPLICATE)
setoption_defaultVal(O_BIND, "");
setoption_initVal(O_BIND, "")   setoption_value(O_BIND, "");

*/


static char *version_string = "0.1"

void print_version() {
  fprintf(stdout, "Version: %s\n", version_string);
}


void print_usage() {
  // Also performs a sanity check on the item list
  // Things not checked:
  //   - duplicate defined options
  //   - multiple usage of items
  //   - order of elements... e.g., this is done last
  //     - have the application do it.
  struct option_item *item;


  fprint(stdout, "Usage:\n");
  fprint(stdout, "scgid [options] [ --port <number> | --socket <file> ] program\n");
  fprint(stdout, "scgid [ --version ] [ --help ]\n");
  fprint(stdout, "scgid [ --conf <file> ]\n");
  fprint(stdout, "\n");

  fprint(stdout, "Options:\n");
  setoption_print_options();
  exit(0);
}



static int configure_options(int *argc_p, char *argv[]) {
  int  option_id;   // the index...

  // Set any special conditions on command-line options
  setoption_options(handle, initVal, defaultVal, duplicative);

  while (option_id = scanopt_nextop(argc_p, argv, option_list, option)) != -1) {
    switch (option_id) {

      case o_CONF:
         // See above, This option is handled as a special case
         fprintf(stderr, "--config must be the only command line option\n");
         exit(1);
         break;

      ///////////////////////////////////////////////////
      //  Script Related Options

      case o_DOCUMENTS:
        // We could move the test for is directory
        // when we build the final scriptname
        // 
        {
          struct stats;

          retVal = stat(option.value, &stats);
          if (retVal != 0) {
            perror("")
            exit(1);
          }
          retVal = S_ISDIR(stats.st_mode);
          if (retVal != 0) {
            perror("")
            exit(1);
          }       
        }
        document_root = option.value;
        break;

      case o_EXEC:
        // although o_DOCUMENT is define
        // document_root might not be
        // so we have to what to test for validity
        scriptname = option.value;
        break;


      ///////////////////////////////////////////////////
      // Daemon Related Options

      case o_FOREGROUND:
        foreground_flag  = option_value.flag;
        break;

      case o_USER:
        {
           struct passwd *record;

            record = getpwnam(option.value);
            if (record == NULL) {
              fprintf(stderr, "Invalid user assocated with --user\n");
              exit(1);
            }
            user = record.pw_uid;
        }
        break;

      case o_WORKERS:
        num_workers = option_value.iVal;
        if (num_workers < 0 || num_workers > WORKERS_MAX) {
          num_workers = WORKERS_DEFUALT;
          fprintf(stderr, "Number of workers out of range, setting to %d\n", num_workers);
        break;


      ///////////////////////////////////////////////////
      // Information Related Options
      case o_HELP:
         help_flag = isSet(item);
         break;
   
      case o_VERSION:
         version_flag = option_value.flag;
         break;


      ///////////////////////////////////////////////////
      // Logging Related Options
      case o_LOG:
        error_log = fopen(opt_value.sVal,"a");
        if (error_log == NULL) {
          perror("Unable to open log file: %s\n", option.longName);
          exit(1)
        }
        break;

      case o_VERBOSE:
        verbose_flag = option_value.flag;
        break;


      ///////////////////////////////////////////////////
      // Network Related Options
      case o_BIND:
        bind_flag = FLAG_SET;

        if (socket_file != NULL) {
          fprintf(stderr, "Illegal use of both --bind and --socket\n");
          exit(1);
        }
        { 
          int retVal;
          in_addr_t s_addr;

          retVal = inet_pton(AF_INET, server_ip, &s_addr);
          if (retVal == 0) {
            perror("IP address (%s) is not parsable", server_ip);
            exit(1);
          }
        }
        break;

      case o_PORT:
        if (socket_file != NULL) { 
          fprintf(stderr, "Illegal use of both --port and --socket\n");
          exit(1);
        }        
        port_num = opt_value.iVal;
        break;

      case o_SOCKET:
        if (bind_flag) { 
          fprintf(stderr, "Illegal use of both --bind and --socket\n");
          exit(1);
        }        
        if (port_num != PORT_UNDEFINED) { 
          fprintf(stderr, "Illegal use of both --port and --socket\n");
          exit(1);
        }
        socket_file = option.value;
        break;

      ///////////////////////////////////////////////////

      default:
        // error
        print_usage();
        exit(1);
    }
  }

  if (version_flag || help_flag) {
    if (version_flag) print_version();
    if (help_flag) print_usage();
    exit(0);
  }


  return optind;  //(the next optind to process)
                  // this should be "[ exec ]"
}



