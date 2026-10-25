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


// Initial values included for documentation.
// values will be initialized via scanopt_initialize
// and subsequently updated via scanopt_scan, etc

char  *server_ip = NULL;       // o_BIND
int    port = 0;               // o_PORT
char  *filter_name = NULL;     // o_FILTER
int    socket_flag = 0;        // o_SOCKET
int    user = 0;               // o_USER
int    foreground_flag = 0;    // o_FOREGROUND
int    verbose_level = 0;      // o_VERBOSE
int    workers = 0;            // o_WORKERS
int    help_flag = 0;          // o_HELP
int    version_flag = 0;       // o_VERSION
char  *scgi_dir = NULL;        // e_SCGI_DIR


// SUBROUTINE Declarations:
extern void set_command_line_options(int argc, char *argv[]);

static void configure_options(option_t option_list[]);
  // extends the definition to include extended functionality
  // provided by scanopt (over getopt_long)
  // two versions are provided
  //   1. an initialized array of option_t, akin to getopt_long
  //   1. incremental calls to build an array of option_t
  // both are provided to
  //   1. test the scanopt library
  //   1. provide an example of scanopt library
  //   1. practice which one provides the most clarity
  // if the extended functionaly of scanopt is not be used
  // the initialized array of option_t is more readable

static void apply_option_definitions(int option_id);
  // the switch associated with the traditional
  //
  //    while ( ch = getopt_long(...) ) != -1 ))
  //
  // albeit with `scanopt_next` instead
  // 
  // done for code clarity

static char * define_scgi_dir(void);
static void source_config_file(char *filename, FILE *fp);

static void print_version(void);
static void print_usage(void);
static void print_usage_header(char *indent);


#ifndef ALTERNATIVE

static option_t option_list[] = {
// id,           argType,           long,        short,  envName
  {o_CONF,       required_argument, "config"      },  // This option MUST be listed first
  {o_BIND,       required_argument, "bind",       "b",    "SCGI_BIND" },
  {o_FOREGROUND, no_argument,       "foreground", "f",    "SCGI_FOREGROUND" },
  {o_HELP,       no_argument,       "help",       "h"     },       
  {o_PORT,       required_argument, "port",       "p",    "SCGI_PORT" },
  {o_SOCKET,     no_argument,       "socket",     NULL,   "SCGI_SOCKET" },
  {o_UDS,        no_argument,       "uds"         },
  {o_FILTER,     required_argument, "filter",     NULL,   "SCGI_FILTERNAME" },
  {o_WORKERS,    required_argument, "workers",    "w",    "SCGI_WORKERS" },
  {o_USER,       required_argument, "user",       "u",    "SCGI_USER" },
  {o_VERBOSE,    optional_argument, "verbose",    "v",    "SCGI_VERBOSE" },
  {o_VERSION,    no_argument,       "version",    "V"     },      
  {e_SCGI_DIR,  required_argument, NULL,         NULL,    "SCGI_DIR" },
  SO_OPTIONS_END()
};

#else

static option_t option_list[MAX_OPTIONS];

#endif


extern void set_command_line_options(int argc, char *argv[]) {
  int      option_id;
  int      next_arg;
  option_t *item;

  //extern option_t option_list[];

  configure_options(option_list);

  next_arg = scanopt_scan(argc, argv, option_list);
  // assert: all command line options have been scanned
  //       : no env have been applied

  // scanopt_apply_environment(): 
  //    could be called to apply all environment variables
  //    note that they are incrementally applied with scanopt_next

  ///////////////////////////////////////////////
  // Boot strap step:
  //   the scgi_dir is determined scanopt_next
  //   but we have to source the config file 
  //   first, when there are NO command-line options
  //
  //   hence we bootstrap `scgi_dir` here
  //   with it potentially be overridend later
  scgi_dir = define_scgi_dir();

  // SPECIAL CASE for command line processing
  // an implicit call to  `--config $SCGI_HOME/config`
  if (next_arg == 1) {
    FILE *fp;
    char config_file[FILENAME_MAX];

    strlcpy(config_file, scgi_dir, FILENAME_MAX);
    strlcat(config_file, DEFAULT_CONFIG_FILE, FILENAME_MAX);

    fp = fopen(config_file, "r");
    if (fp != NULL) {
      source_config_file(config_file, fp);
      fclose(fp);
    }
  }

  // For each option, apply the cached definitions
  while ((option_id = scanopt_next(option_list)) != -1) {
    apply_option_definitions(option_id);
  }

  if (version_flag == SET || help_flag == SET) {
    if (version_flag == SET) print_version();
    if (help_flag == SET)    print_usage();
    exit(0);
  }

  // Sanity checks:
  //  1. must have either socket or port
  if (port == 0 && socket_flag == UNSET) {
    fprintf(stderr, "Either the --port or the --socket option needs to be provided.\n");
    exit(1);
  }

  if (argc - next_arg != 0) {
      fprintf(stderr, "Superfluous command line arguments encountered\n");
      exit(1);
  }
  return;
}


static void source_config_file(char *filename, FILE *fp) {
  char   *buffer;
  size_t capacity;
  int    length;

  fprintf(stdout, "Sourcing config file: %s\n", filename);
  buffer = NULL; capacity = 0;
  length = getline(&buffer, &capacity, fp);

  while (length > 0) {
    assert(buffer[length-1] == '\n');

    buffer[length-1] = '\0';
    putenv(buffer);

    buffer=NULL; capacity=0;
    length = getline(&buffer, &capacity, fp);
  }
  return;
}


static void print_version(void) {

  fprintf(stdout, "Version: %s\n", version_string);
  return;
}


static void print_usage(void) {
  char *indent = "  ";

  print_usage_header(indent);
  scanopt_usage_options(option_list, indent);
  scanopt_usage_envs(option_list, indent);
  exit(0);
}


static void print_usage_header(char *indent) {

  fprintf(stdout, "\nUsage:\n");
  fprintf(stdout, "%sscgid [options] ( --port <number> | --socket ) [ program ]\n", indent);
  fprintf(stdout, "%sscgid [ --version ] [ --help ] [ ignored ]\n", indent);
  fprintf(stdout, "%sscgid [ --conf <file> ]\n", indent);
  fprintf(stdout, "\n");
  return;
}


static void apply_option_definitions(int option_id) {
  int x;
  switch (option_id) {
    case e_SCGI_DIR:
      // it's okay if this value gets redefined
      scgi_dir = optarg;
      break;

    case o_CONF:
      {
        FILE *fp;
     
        if (optcount > 1) {
          // this is only for scgid, not a general requirement
          fprintf(stderr, "--config must be the only command line option\n");
          exit(1);
        }

        fp = fopen(optarg, "r");
        if (fp == NULL) {
          fprintf(stderr, "%s", optarg);
          perror(optarg);
          exit(1);
        }
        source_config_file(optarg, fp);
        fclose(fp);
      }
      break;


    case o_FILTER:
      {
        struct stat stats;

        if (stat(optarg, &stats) != 0) {
         perror("filter name is not a valid filename.");
         exit(1);
        }
        if (S_ISREG(stats.st_mode) != 0) {
          perror("filter name is not a regular file.");
          exit(1);
        }
      }
      filter_name = optarg;
      break;

    ///////////////////////////////////////////////////
    // Daemon Related Options

    case o_FOREGROUND:
      foreground_flag  = SET;
      // This can be handled via a scanopt variable
      break;

    case o_USER:
      assert(false);
      fprintf(stderr, "Server not running as root. Hence unable to change effective uid to: %s", optarg);
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
       //version_flag = SET;
       // This can be handled via a scanopt variable
       break;

    case o_VERBOSE:
      verbose_level = optvalue.iVal;
      // This can be handled via a scanopt variable
      break;


    ///////////////////////////////////////////////////
    // Network Related Options
    case o_BIND:
      if (socket_flag == SET) {
        fprintf(stderr, "Illegal use of both --bind and --socket options.\n");
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
      // Isn't  there an issue of the SOCKET and UDS have differe values
      // well not in this case, but what in general

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
      print_usage();
      exit(1);
  }
  return;
}


#ifndef ALTERNATIVE

// See Above for
// static option_t option_list[] = { ... };

static void configure_options(option_t option_list[]) {
  option_t *item;

  // o_CONF: 
  item = scanopt_find(o_CONF, option_list);
  scanopt_assign_usage(item, "filename", "The name of the configuration file");

   // o_BIND: server_IP
  item = scanopt_find(o_BIND, option_list);
  scanopt_assign_usage(item, "ip_v4", "The ip address associated with the server side socket");

  // o_FOREGROUND: foreground_flag
  item = scanopt_find(o_FOREGROUND, option_list);
  scanopt_assign_usage(item, NULL, "Have the server run in the foreground");
  scanopt_assign_flag(item, (void *) &foreground_flag);

  // o_HELP: help_flag
  item = scanopt_find(o_HELP, option_list);
  scanopt_assign_usage(item, NULL, "Display a short summary of available command line options.");
  scanopt_assign_flag(item, (void *) &help_flag);

  // o_PORT: port
  item = scanopt_find(o_PORT, option_list);
  scanopt_assign_usage(item, "int", "The TCP/IP port associated with the server side socket");

  // o_SOCKET: socket_flag
  item = scanopt_find(o_SOCKET, option_list);
  scanopt_assign_usage(item, NULL, "Have the server use a UDS (unix domain socket) $SCGI_HOME/socketfile");

  // o_UDS: socket_flag
  item = scanopt_find(o_UDS, option_list);
  scanopt_assign_usage(item, NULL, "Have the server use a UDS (unix domain socket) $SCGI_HOME/socketfile");

  // o_Filter: filter_name
  item = scanopt_find(o_FILTER, option_list);
  scanopt_assign_usage(item, "executable", "A program that can filter the input and output of the cgi script");

  // o_WORKERS: workers
  item = scanopt_find(o_WORKERS, option_list);
  scanopt_assign_usage(item, "int", "The number of (initial) workers that field network requests");
  scanopt_assign_variable(item, NULL, SO_VAL_INT, "5", NULL);  // Initial value is 5 workers

  // o_USER: user
  item = scanopt_find(o_USER, option_list);
  scanopt_assign_usage(item, "name", "The user id associated with the daemon and workers. Requires server to run as root.");

  // o_VERBOSE: verbose_level
  item = scanopt_find(o_VERBOSE, option_list);
  scanopt_assign_usage(item, "number", "Increase the verbosity of logging");
  scanopt_assign_variable(item, NULL, SO_VAL_INT, "0", "1");  // Initial value is 0 for no logging, and "1" is the default value.

  // o_VERSION: version_flag
  item = scanopt_find(o_VERSION, option_list);
  scanopt_assign_usage(item, NULL, "Displays the current version of the software");
  scanopt_assign_flag(item, (void *) &version_flag);

  // e_SCGI_DIR: scgi_dir
  item = scanopt_find(e_SCGI_DIR, option_list);
  scanopt_assign_usage(item, NULL, "The home directory of the SCGI server");
  //scanopt_assign_flag(item, (void *) &scgi_dir);

  return;
}

#else  // ALTERNATE

// See Above for
// static option_t option_list[MAX_OPTIONS];

static void configure_options(option_t option_list[]) {
  option_t *item;

  // o_CONF:  // order matters:  o_CONF must be first
  item = scanopt_declare_option(option_list, o_CONF, required_argument, "config", NULL, NULL, MAX_OPTIONS);
  scanopt_assign_usage(item, "filename", "The name of the configuration file");

   // o_BIND: server_IP
  item = scanopt_declare_option(option_list, o_BIND, required_argument, "bind", "b", "SCGI_BIND", MAX_OPTIONS);
  scanopt_assign_usage(item, "ip_v4", "The ip address associated with the server side socket");

  // o_FOREGROUND: foreground_flag
  item = scanopt_declare_option(option_list, o_FOREGROUND, no_argument, "foreground", "f", "SCGI_FOREGROUND", MAX_OPTIONS);
  scanopt_assign_usage(item, NULL, "Have the server run in the foreground");
  scanopt_assign_flag(item, (void *) &foreground_flag);

  // o_HELP: help_flag
  item = scanopt_declare_option(option_list, o_HELP, no_argument, "help", "h", NULL, MAX_OPTIONS);
  scanopt_assign_usage(item, NULL, "Display a short summary of available command line options.");
  scanopt_assign_flag(item, (void *) &help_flag);

  // o_PORT: port
  item = scanopt_declare_option(option_list, o_PORT, required_argument, "port", "p", "SCGI_PORT", MAX_OPTIONS);
  scanopt_assign_usage(item, "int", "The TCP/IP port associated with the server side socket");

  // o_SOCKET: socket_flag
  item = scanopt_declare_option(option_list, o_SOCKET, no_argument, "socket", NULL, "SCGI_SOCKET", MAX_OPTIONS);
  scanopt_assign_usage(item, NULL, "Have the server use a UDS (unix domain socket) $SCGI_HOME/socketfile");

  // o_UDS: socket_flag
  item = scanopt_declare_option(option_list, o_UDS, no_argument, "uds", NULL, NULL, MAX_OPTIONS);
  scanopt_assign_usage(item, NULL, "Have the server use a UDS (unix domain socket) $SCGI_HOME/socketfile");

  // o_Filter: filter_name
  item = scanopt_declare_option(option_list, o_FILTER, required_argument, "filter", NULL, "SCGI_FILTERNAME", MAX_OPTIONS);
  scanopt_assign_usage(item, "filename", "A filter script that process the inbound and oubound data associated with cgi program" );

  // o_WORKERS: workers
  item = scanopt_declare_option(option_list, o_WORKERS, required_argument, "workers", "w", "SCGI_WORKERS", MAX_OPTIONS);
  scanopt_assign_usage(item, "int", "The number of (initial) workers that field network requests");
  scanopt_assign_variable(item, NULL, SO_VAL_INT, "5", NULL);  // Initial value is 5 workers

  // o_USER: user
  item = scanopt_declare_option(option_list, o_USER, required_argument, "user", "u", "SCGI_USER", MAX_OPTIONS);
  scanopt_assign_usage(item, "name", "The user id associated with the daemon and workers. Requires server to run as root.");

  // o_VERBOSE: verbose_level
  item = scanopt_declare_option(option_list, o_VERBOSE, optional_argument, "verbose", "v", "SCGI_VERBOSE", MAX_OPTIONS);
  scanopt_assign_usage(item, "number", "Increase the verbosity of logging");
  scanopt_assign_variable(item, NULL, SO_VAL_INT, "0", "1");  // Initial value is 0 for no logging, and "1" is the default value.

  // o_VERSION: version_flag
  item = scanopt_declare_option(option_list, o_VERSION, no_argument, "version", "V", NULL, MAX_OPTIONS);
  scanopt_assign_usage(item, NULL, "Displays the current version of the software");
  scanopt_assign_flag(item, (void *) &version_flag);

  // e_SCGI_DIR: scgi_dir
  item = scanopt_declare_option(option_list, e_SCGI_DIR, no_argument, NULL, NULL, "SCGI_HOME", MAX_OPTIONS);
  scanopt_assign_usage(item, NULL, "The home directory of the SCGI server");
  //scanopt_assign_flag(item, (void *) &scgi_dir);

  return;
}

#endif


static char * define_scgi_dir(void) {
  // Determine the value of scgi_dir;
  //   1. /tmp/
  //   2. $HOME/.scgi/, if $HOME
  //   3. $SCGI_HOME/, if $SCGI_HOME
  char * buffer;
  char * env_p;

  buffer = malloc(sizeof(char)*FILENAME_MAX);
  strlcpy(buffer, "/tmp/", FILENAME_MAX);

  env_p = getenv("HOME");
  if (env_p != NULL) {
    strlcpy(buffer, env_p, FILENAME_MAX);
    strlcat(buffer, "/.scgi/", FILENAME_MAX);
  }
  env_p = getenv("SCGI_HOME");
  if (env_p != NULL) {
    strlcpy(buffer, env_p, FILENAME_MAX);
    strlcat(buffer, "/", FILENAME_MAX);
  }
  return buffer;
}

