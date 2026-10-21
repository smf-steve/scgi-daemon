
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/socket.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "scanopt.h"
#include "command_line.h"

static char *version_string = "0.1";

#define SET (1)
#define UNSET (0)


//# SO_ARG_TYPE(otype, type, duplicative);
//# SO_ARG_TYPE(required, text, error);


static option_t option_list[] = {
// id,           longName,     shortName, argType
  {o_BIND,       "bind",       "b",       required_argument},
  {o_CONF,       "config",     NULL,      required_argument},
  {o_DOCUMENTS,  "documents",  "d",       required_argument},
  {o_FOREGROUND, "foreground", "f",       no_argument},
  {o_HELP,       "help",       "h",       no_argument},
  {o_LOG,        "log",        "l",       required_argument},
  {o_PORT,       "port",       "p",       required_argument},
  {o_SOCKET,     "socket",     "s",       required_argument},
  {o_UDS,        "uds",        NULL,      required_argument},
  {o_EXEC,       "exec",       "e",       required_argument},
  {o_WORKERS,    "workers",    "w",       required_argument},
  {o_USER,       "user",       "u",       no_argument},
  {o_VERBOSE,    "verbose",    "v",       no_argument},
  {o_VERSION,    "version",    "V",       no_argument},
  SO_OPTIONS_END()
};

// The following are variables related to options. There
// values are set via `process_cli_options` and used with `main`

char * server_ip;
char * document_root;
int    foreground_flag;
char * port = NULL;
int    port_num = 0;
char * scriptname;
char * socket_file = NULL;
int    user;
int    verbose_flag;
int    workers;
int    help_flag;
int    version_flag;


static void print_version();
static void print_usage(option_t option_list[]);
static void print_usage_header();
static int strprefix(char *prefix, char * str);

// calling function:
static char * parse_config_file(int argc, char **argv, char *home);
static void source_config_file(char * filename);
///
static int parse_options(int argc, char *argv[]);


////////////////////////////////////////////
// COMMAND LINE and ENV PROCESSING
// 
//   scgi [ --conf[=file ]]
//
//     1. special case for using the --conf option
//        - if present, it must be the only option
//        - it is implicitly present, if the command line is empty
//
//   scgi [options] ( --port=<number> | --socket=<filename> ) [ exec ]
//
//      2. normal processing of the command line for options
//         - i.e., the `[ exec ]` component is handled in step 4.
//   
//   scgi [--version | --help ] [ ignored ]
//
//      3. special case with use of --version and --help
//         - regardless of the composition of the command line
//         - if the --version or --help is present: 
//
// 4. check if there is only one parameter left ( [ exec ] )
//


extern void set_command_line_options(int argc, char *argv[], char *scgi_home) {

  char config_file[FILENAME_MAX];
  int num_options;


  // 1. Get the name of the config file from the command line, and source it
  strlcpy(config_file, scgi_home, FILENAME_MAX);
  if (parse_config_file(argc, argv, config_file) != NULL) {
    source_config_file(config_file);
    //scanopt_apply_env();
    argc = 0;
    argv[0] = NULL;
  }


  // 2. Process the Command Line Options, which include environment variables
  num_options = parse_options(argc, argv);

  // 3. Note that if the special case of either --version or --help was detected,
  //    the above parse_command_line handled it with exit(0) performed.
  //    : scgi  --version --help

  // 4. Check if any command line parameters exist for '[ exec ]'
  switch (argc - num_options) {
    case 0:
      // The command line is effectively empty.
      break;

    case 1:
      // The command line contained just '[ exec ]'
      if (scriptname != NULL) {
        fprintf(stderr, "Duplicate scriptname provided: %s and %s\n", scriptname, argv[num_options]);
        exit(1);
      }
      scriptname = argv[num_options];
      fprintf(stderr, "Warning: Using perpetual script: %s\n", scriptname);
      break;

    default:
      fprintf(stderr, "Superfluous command line arguments encountered\n");
      exit(1);
  }
  ////////////////////////////////////////////

  ///////////////////////////////////////////
  //
  if (scriptname != NULL) {
    {
      struct stat stats;

      if (stat(optarg, &stats) != 0) {
        perror("scriptname is not a valid filename.");
        exit(1);
      }
      if (S_ISREG(stats.st_mode) != 0) {
        perror("scriptname is not a regular file.");
        exit(1);
      }
    }
  }

  return;
  /// END of COMMAND LINE PROCESSING
  //////////////////////////////////////////////////

}


char * parse_config_file(int argc, char **argv, char *home) {

  char *filename = NULL;

  switch (argc) {
    case 1:   // Command Line:  argv[0]
      strlcat(home, "/conf", FILENAME_MAX);
      filename = home;
      break;

    case 2:   // Command Line:   argv[0] --conf=filename
      if (strprefix("--conf=", argv[1])) {
        filename = index(argv[1], '=') + 1;
      }
      break;

    case 3:   // Command Line:   argv[0] --conf filename
      if (strcmp("--conf", argv[1]) == 0) {
        filename = argv[2];
      }
      break;

    default:
      break;
  }

  return filename;
}


void source_config_file(char * filename) {
  ////////////////////////////////////////////
  //   Source the Configuration File
  FILE *fp;

  char *buffer;
  size_t capacity;
  int  length;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    char str[FILENAME_MAX];
    bzero(str, FILENAME_MAX);
    sprintf(str, "Unable to open config file: %s\n", filename);
    perror(str);
    print_usage(option_list);
    exit(1);
  }

  length = getline(&buffer, &capacity, fp);
  // might have an issue if the last line of the config is not a "\n"
  while (length > 0) {
    assert(buffer[length] == '\n');
    buffer[length] = '\0';
    putenv(buffer);
    length = getline(&buffer, &capacity, fp);
  }
  fclose(fp);
}




void print_version() {
  fprintf(stdout, "Version: %s\n", version_string);
}

void print_usage_header() {
  fprintf(stdout, "\nUsage:\n");
  fprintf(stdout, "scgid [options] ( --port <number> | --socket <file> ) [ program ]\n");
  fprintf(stdout, "scgid [ --version ] [ --help ] [ ignored ]\n");
  fprintf(stdout, "scgid [ --conf <file> ]\n");
  fprintf(stdout, "\n");
  return;
}

void print_usage(option_t option_list[]) {
  // Also performs a sanity check on the item list
  // Things not checked:
  //   - duplicate defined options
  //   - multiple usage of items
  //   - order of elements... e.g., this is done last
  //     - have the application do it.

  print_usage_header();
  scanopt_usage(option_list);
  exit(0);
}


extern int parse_options(int argc, char *argv[]) {
  int  option_id;
  int  num_options;
  option_t * item;

  // Set any special conditions on command-line options
  //scanopt_config();


  // scanopt_validate(option_list);
  // scanopt_initialize(option_list);
  num_options = scanopt_scan(argc, argv, option_list);

  while ((option_id = scanopt_next(option_list)) != -1) {
    switch (option_id) {

      case o_CONF:
         // This option is handled as a special case.
         // Hence, it should not appear in this general case.
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
          struct stat stats;

          if (stat(optarg, &stats) != 0) {
            perror("document value is not a valid filename");
            exit(1);
          }
          if (S_ISDIR(stats.st_mode) != 0) {
            perror("document value is not a valid directory");
            exit(1);
          }
        }
        document_root = optarg;
        break;

      case o_EXEC:
        // Validation of scriptname is deferred
        // scriptname may be the first (and only) argument
        scriptname = optarg;
        break;

      ///////////////////////////////////////////////////
      // Daemon Related Options

      case o_FOREGROUND:
        foreground_flag  = SET;
        // This can be handled via a scanopt variable
        break;

      case o_USER:
        {
           struct passwd *record;

            record = getpwnam(optarg);
            if (record == NULL) {
              fprintf(stderr, "Invalid user assocated with --user\n");
              exit(1);
            }
            user = record->pw_uid;
        }
        break;

      case o_WORKERS:
        workers = atoi(optarg);    /// CONVERSION
        if (workers < 0 || workers > WORKERS_MAX) {
          workers = WORKERS_DEFAULT;
          fprintf(stderr, "Number of workers out of range, setting to %d\n", workers);
        }
        break;

      ///////////////////////////////////////////////////
      // Information Related Options
      case o_HELP:
         help_flag = SET;
         // This can be handled via a scanopt variable

         break;
   
      case o_VERSION:
         version_flag = SET;
         // This can be handled via a scanopt variable
         break;

      ///////////////////////////////////////////////////
      // Logging Related Options
      case o_LOG:
        {
          FILE * fp;
          fp = freopen(optarg, "w", stderr);
          if (fp == 0) {
            perror("Unable to open logfile\n");
          }
        }
        break;

      case o_VERBOSE:
        verbose_flag = SET;
        // This can be handled via a scanopt variable
        break;

      ///////////////////////////////////////////////////
      // Network Related Options
      case o_BIND:
        if (socket_file != NULL) {
          fprintf(stderr, "Illegal use of both --bind and --socket options.\n");
          print_usage_header();
          exit(1);
        }
        {
          in_addr_t s_addr;

          if (inet_pton(AF_INET, optarg, &s_addr) == 0) {
            perror("IP address is not parsable");
            //perror("IP address (%s) is not parsable", server_ip);
            exit(1);
          }
        }
        server_ip = optarg;
        break;

      case o_PORT:
        if (socket_file != NULL) {
          fprintf(stderr, "Illegal use of both --port and --socket\n");
          exit(1);
        }
        port = optarg;
        port_num = atoi(optarg);  // need to convert
        break;

      case o_SOCKET:
        if (server_ip != NULL) {
          fprintf(stderr, "Illegal use of both --bind and --socket\n");
          exit(1);
        }
        if (port != NULL) {
          fprintf(stderr, "Illegal use of both --port and --socket\n");
          exit(1);
        }
        socket_file = optarg;

        break;

      ///////////////////////////////////////////////////
      case SO_ID_ERROR:
          // replacement for ':' '?' and/or 1
          // case ':': fprintf(stderr, ": option\n"); exit(2);
          // case '?': fprintf(stderr, "? option\n"); exit(2);

      default:
        // error
        print_usage(option_list);
        exit(1);
    }
  }

  if (version_flag == SET || help_flag == SET) {
    if (version_flag == SET) print_version();
    if (help_flag == SET) print_usage(option_list);
    exit(0);
  }

  // Sanity checks:
  //  1. must have either socket or port
  if (port == NULL && socket_file == NULL) {
    fprintf(stderr, "Either the --port or the --socket option needs to be provided.\n");
    print_usage_header();
    exit(1);
  }

  return num_options;  //(the next optind to process)
                       // this should be "[ exec ]"
}




static int strprefix(char *prefix, char * str) {
   // returns true if str is equivalent to prefix....
   int c;
   for (c = 0; prefix[c] != '\0'; c++) {
     if (str[c] == '\0') break;
     if (prefix[c] != str[c]) break;
   }

   return (prefix[c] == '\0');
}

