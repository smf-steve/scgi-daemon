
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

#define DEFAULT_CONFIG_FILE   "config"
#define DEFAULT_WORKERS (5)
#define MAX_WORKERS (15)


//# SO_ARG_TYPE(otype, type, duplicative);
//# SO_ARG_TYPE(required, text, error);


static option_t option_list[] = {
// id,           longName,     shortName, argType
  {o_BIND,       "bind",       "b",       required_argument},
  {o_CONF,       "config",     NULL,      required_argument},
  {o_FOREGROUND, "foreground", "f",       no_argument},
  {o_HELP,       "help",       "h",       no_argument},
  {o_PORT,       "port",       "p",       required_argument},
  {o_SOCKET,     "socket",     NULL,      no_argument},
  {o_UDS,        "uds",        NULL,      no_argument},
  {o_SCRIPT,     "script",     NULL,      required_argument},
  {o_WORKERS,    "workers",    "w",       required_argument},
  {o_USER,       "user",       "u",       no_argument},
  {o_VERBOSE,    "verbose",    "v",       no_argument},
  {o_VERSION,    "version",    "V",       no_argument},
  SO_OPTIONS_END()
};

// The following are variables related to options. There
// values are set via `process_cli_options` and used with `main`

char * server_ip       = NULL;
int    port            = 0;

char * scriptname;
int    socket_flag     = UNSET;
int    user;
int    foreground_flag = UNSET;
int    verbose_flag    = UNSET;
int    workers         = DEFAULT_WORKERS;
int    help_flag       = UNSET;
int    version_flag    = UNSET;


static void print_version();
static void print_usage(option_t option_list[]);
static void print_usage_header();

// calling function:
static void source_config_file(char * filename);
///
static int parse_options(int argc, char *argv[]);


extern void set_command_line_options(int argc, char *argv[], char *scgi_dir) {

  int num_options;

  if (argc == 1) {
    // the command line is empty, hence invoke the config file.
    char config_file[FILENAME_MAX];

    strlcpy(config_file, scgi_dir, FILENAME_MAX);
    strlcat(config_file, DEFAULT_CONFIG_FILE, FILENAME_MAX);
    source_config_file(config_file);
  } 

  num_options = parse_options(argc, argv);

  // 4. Check if any command line parameters exist for '[ exec ]'
  switch (argc - num_options) {
    case 0:
      // There are no arguments to the command
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
  fprintf(stdout, "scgid [options] ( --port <number> | --socket ) [ program ]\n");
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
        if (optcount > 1) {
          fprintf(stderr, "--config must be the only command line option\n");
           exit(1);
        }
        source_config_file(optarg);
        break;

      ///////////////////////////////////////////////////
      //  Script Related Options

      case o_SCRIPT:
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
              fprintf(stderr, "Invalid user associated with --user\n");
              exit(1);
            }
            user = record->pw_uid;
        }
        break;

      case o_WORKERS:
        workers = atoi(optarg);    /// CONVERSION
        if (workers < 0 || workers > MAX_WORKERS) {
          workers = DEFAULT_WORKERS;
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

      case o_VERBOSE:
        verbose_flag = SET;
        // This can be handled via a scanopt variable
        break;

      ///////////////////////////////////////////////////
      // Network Related Options
      case o_BIND:
        if (socket_flag == SET) {
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
        if (socket_flag == SET) {
          fprintf(stderr, "Illegal use of both --port and --socket\n");
          exit(1);
        }
        port = atoi(optarg);  // need to convert
        break;

      case o_SOCKET:
      case o_UDS:
        if (server_ip != NULL) {
          fprintf(stderr, "Illegal use of both --bind and --socket\n");
          exit(1);
        }
        if (port != 0) {
          fprintf(stderr, "Illegal use of both --port and --socket\n");
          exit(1);
        }
        socket_flag = SET;

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
  if (port == 0 && socket_flag == UNSET) {
    fprintf(stderr, "Either the --port or the --socket option needs to be provided.\n");
    print_usage_header();
    exit(1);
  }

  return num_options;  //(the next optind to process)
                       // this should be "[ exec ]"
}



