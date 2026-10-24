
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


// The following are variables related to options. Their
// values set within this file and used with `main`

//char * server_ip;       // o_BIND
//int    port;            // o_PORT
//char * filter_name;     // o_FILTER
//int    socket_flag;     // o_SOCKET
//int    user;            // o_USER
//int    foreground_flag; // o_FOREGROUND
//int    verbose_level;   // o_VERBOSE
//int    workers;         // o_WORKERS
//int    help_flag;       // o_HELP
//int    version_flag;    // o_VERSION

// initialize until we finish debugging;

char * server_ip = NULL;    // o_BIND
int port = 0;            // o_PORT
char *filter_name = NULL;  // o_FILTER
int socket_flag = 0;     // o_SOCKET
int user = 0;            // o_USER
int foreground_flag = 0; // o_FOREGROUND
int verbose_level = 0;   // o_VERBOSE
int workers = 0;         // o_WORKERS
int help_flag = 0;       // o_HELP
int version_flag = 0;    // o_VERSION


#ifndef ALTERNATIVE

static option_t option_list[] = {
// id,           argType,           longName,     shortName,  envName
  {o_CONF,       required_argument, "config"      },  // This option MUST be listed first
  {o_BIND,       required_argument, "bind",       "b",        "SCGI_BIND" },
  {o_FOREGROUND, no_argument,       "foreground", "f",        "SCGI_FOREGROUND" },
  {o_HELP,       no_argument,       "help",       "h"         },       
  {o_PORT,       required_argument, "port",       "p",        "SCGI_PORT" },
  {o_SOCKET,     no_argument,       "socket",     NULL,       "SCGI_SOCKET" },
  {o_UDS,        no_argument,       "uds"         },
  {o_FILTER,     required_argument, "filter",     NULL,       "SCGI_FILTERNAME" },
  {o_WORKERS,    required_argument, "workers",    "w",        "SCGI_WORKERS" },
  {o_USER,       required_argument, "user",       "u",        "SCGI_USER" },
  {o_VERBOSE,    optional_argument, "verbose",    "v",        "SCGI_VERBOSE" },
  {o_VERSION,    no_argument,       "version",    "V"         },      
  {o_ENV,        required_argument, "",          "hh",      "TEST_ENV" },
  SO_OPTIONS_END()
};


void configure_options(option_t option_list[]) {
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

  return;
}


#else  // ALTERNATE

#define SIZE 20
static option_t option_list[SIZE];

void configure_options(option_t option_list[]) {

  option_t *item;

  // o_CONF:  // order matters:  o_CONF must be first
  item = scanopt_declare_option(option_list, o_CONF, required_argument, "config", NULL, NULL, SIZE);
  scanopt_assign_usage(item, "filename", "The name of the configuration file");

   // o_BIND: server_IP
  item = scanopt_declare_option(option_list, o_BIND, required_argument, "bind", "b", "SCGI_BIND", SIZE);
  scanopt_assign_usage(item, "ip_v4", "The ip address associated with the server side socket");

  // o_FOREGROUND: foreground_flag
  item = scanopt_declare_option(option_list, o_FOREGROUND, no_argument, "foreground", "f", "SCGI_FOREGROUND", SIZE);
  scanopt_assign_usage(item, NULL, "Have the server run in the foreground");
  scanopt_assign_flag(item, (void *) &foreground_flag);

  // o_HELP: help_flag
  item = scanopt_declare_option(option_list, o_HELP, no_argument, "help", "h", NULL, SIZE);
  scanopt_assign_usage(item, NULL, "Display a short summary of available command line options.");
  scanopt_assign_flag(item, (void *) &help_flag);

  // o_PORT: port
  item = scanopt_declare_option(option_list, o_PORT, required_argument, "port", "p", "SCGI_PORT", SIZE);
  scanopt_assign_usage(item, "int", "The TCP/IP port associated with the server side socket");


  // o_SOCKET: socket_flag
  item = scanopt_declare_option(option_list, o_SOCKET, no_argument, "socket", NULL, "SCGI_SOCKET", SIZE);
  scanopt_assign_usage(item, NULL, "Have the server use a UDS (unix domain socket) $SCGI_HOME/socketfile");

  // o_UDS: socket_flag
  item = scanopt_declare_option(option_list, o_UDS, no_argument, "uds", NULL, NULL, SIZE);
  scanopt_assign_usage(item, NULL, "Have the server use a UDS (unix domain socket) $SCGI_HOME/socketfile");

  // o_Filter: filter_name
  item = scanopt_declare_option(option_list, o_FILTER, required_argument, "filter", NULL, "SCGI_FILTERNAME", SIZE);
  scanopt_assign_usage(item, "filename", "A filter script that process the inbound and oubound data associated with cgi program" );

  // o_WORKERS: workers
  item = scanopt_declare_option(option_list, o_WORKERS, required_argument, "workers", "w", "SCGI_WORKERS", SIZE);
  scanopt_assign_usage(item, "int", "The number of (initial) workers that field network requests");
  scanopt_assign_variable(item, NULL, SO_VAL_INT, "5", NULL);  // Initial value is 5 workers

  // o_USER: user
  item = scanopt_declare_option(option_list, o_USER, required_argument, "user", "u", "SCGI_USER", SIZE);
  scanopt_assign_usage(item, "name", "The user id associated with the daemon and workers. Requires server to run as root.");

  // o_VERBOSE: verbose_level
  item = scanopt_declare_option(option_list, o_VERBOSE, optional_argument, "verbose", "v", "SCGI_VERBOSE", SIZE);
  scanopt_assign_usage(item, "number", "Increase the verbosity of logging");
  scanopt_assign_variable(item, NULL, SO_VAL_INT, "0", "1");  // Initial value is 0 for no logging, and "1" is the default value.

  // o_VERSION: version_flag
  item = scanopt_declare_option(option_list, o_VERSION, no_argument, "version", "V", NULL, SIZE);
  scanopt_assign_usage(item, NULL, "Displays the current version of the software");
  scanopt_assign_flag(item, (void *) &version_flag);

  return;
}
#endif




static void print_version();
static void print_usage();
static void print_usage_header(char *indent);

// calling function:
static void source_config_file(FILE *fp);
///
static int parse_options(int argc, char *argv[]);


extern void set_command_line_options(int argc, char *argv[], char *scgi_dir) {

  int next_arg;

  configure_options(option_list);

  if (argc == 1) {
    FILE *fp;
    // the command line is empty
    // hence invoke the default config file, if it exists.
    char config_file[FILENAME_MAX];

    strlcpy(config_file, scgi_dir, FILENAME_MAX);
    strlcat(config_file, DEFAULT_CONFIG_FILE, FILENAME_MAX);

    fp = fopen(config_file, "r");
    if (fp == NULL) {
      return;
    } else {
      source_config_file(fp);
      fclose(fp);
    }
  } 

  next_arg = parse_options(argc, argv);

  // 4. Check if any command line parameters exist for '[ exec ]'
  if (argc - next_arg != 0) {
      fprintf(stderr, "Superfluous command line arguments encountered\n");
      exit(1);
  }

  return;
}



void source_config_file(FILE *fp) {
  ////////////////////////////////////////////
  //   Source the Configuration File
  char *buffer;
  size_t capacity;
  int  length;

  buffer=NULL; capacity=0;
  length = getline(&buffer, &capacity, fp);
  // might have an issue if the last line of the config is not a "\n"
  while (length > 0) {
    char * v_str;
    assert(buffer[length-1] == '\n');
    buffer[length-1] = '\0';
    //v_str = strchr(buffer, '=');
    //(* v_str) = '\0';
    //v_str ++;

    //setenv(buffer, v_str, 1);
    putenv(buffer);

    buffer=NULL; capacity=0;
    length = getline(&buffer, &capacity, fp);
  }
}




void print_version() {
  fprintf(stdout, "Version: %s\n", version_string);
}

void print_usage_header(char *indent) {
  fprintf(stdout, "\nUsage:\n");
  fprintf(stdout, "%sscgid [options] ( --port <number> | --socket ) [ program ]\n", indent);
  fprintf(stdout, "%sscgid [ --version ] [ --help ] [ ignored ]\n", indent);
  fprintf(stdout, "%sscgid [ --conf <file> ]\n", indent);
  fprintf(stdout, "\n");
  return;
}

void print_usage() {

  char *indent = "  ";

  print_usage_header(indent);
  scanopt_usage(option_list, indent);
  exit(0);
}


extern int parse_options(int argc, char *argv[]) {
  int  option_id;
  int  next_arg;
  option_t * item;

  next_arg = scanopt_scan(argc, argv, option_list);

  while ((option_id = scanopt_next(option_list)) != -1) {
    switch (option_id) {

      case o_CONF:
        {
          FILE *fp;
        
          if (optcount > 1) {
            fprintf(stderr, "--config must be the only command line option\n");
            exit(1);
          }

          fp = fopen(optarg, "r");
          if (fp == NULL) {
            fprintf(stderr, "%s", optarg);
            perror(optarg);
            exit(1);
          }
          source_config_file(fp);
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

      case o_ENV:
         fprintf(stdout, "Test for o_ENV");
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
  }

  if (version_flag == SET || help_flag == SET) {
    if (version_flag == SET) print_version();
    if (help_flag == SET) print_usage();
    exit(0);
  }

  // Sanity checks:
  //  1. must have either socket or port
  if (port == 0 && socket_flag == UNSET) {
    fprintf(stderr, "Either the --port or the --socket option needs to be provided.\n");
    exit(1);
  }

  return next_arg;    //(the next optind to process)
                      // this should be "[ exec ]"
}



