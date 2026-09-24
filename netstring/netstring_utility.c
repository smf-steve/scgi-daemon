#include "scgi_netstring.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>


/*****************************************************************/
/* File: netstring_utility.c                                     */
/*                                                               */
/* Purpose:                                                      */
/*   - To create a command line utility that facilitate           */
/*     the encoding and decoding of a netstring.                 */
/*   - To utilize said tool to perform system testing.           */
/*                                                               */
/* Usage:                                                        */
/*   netstring [-e] [options] [[env_file] [netstring]]           */
/*   netstring [-d] [options] [[netstring] [env_file]]           */
/*                                                               */
/*                                                               */
/* Options:                                                      */
/*    -e, --encode    : encode a sequence of text strings ('\n'  */
/*                      terminated) into a netstring. (DEFAULT)  */
/*                                                               */
/*    -d, --decode    : decode a netstring into a sequence of    */
/*                      text strings ('\n' terminated)           */
/*                                                               */
/*    --min-length=n  : set the minimum length of the sequence   */
/*                      of text strings.                         */
/*                      (DEFAULT: --min-length=0)                */
/*                                                               */
/*    --max-length=n  : set the maximum length of the sequence   */
/*                      of text strings.                         */
/*                      (DEFAULT value: see below)               */
/*                                                               */
/*    --max-strings=n : set the maximum number of text strings    */
/*                      encoded/decode                           */
/*                      (DEFAULT value: see below)               */
/*                                                               */
/*    --use-files     : use file descriptors (int fd) for I/O    */
/*                      (DEFAULT)                                */
/*                                                               */
/*    --use-streams   : use streams (FILE *fp) for I/O           */
/*                                                               */
/*    --scgi          : validate that netstring conforms to      */
/*                      the SCGI protocol                        */
/*                      sets --min-string-length=11              */ 
/*                                                               */
/*    --timing[=n]    : runs netstring_{f}read `n` times to      */
/*                      support performance timing.  If `n`      */
/*                      is not define, it is set to 0xFFFFF.     */
/*                                                               */
/* Environment Variables:                                        */
/*    In lie of the above command-line options, environment      */
/*    variables can be defined to achieve the same effect. If    */
/*    these variables are not defined, the default value are:   */  
/*                                                               */
/*    NETSTRING_MIN_LENGTH  : 0                                  */
/*    NETSTRING_MAX_LENGTH  : 65,535  (0xFFFF)                   */
/*    NETSTRING_MAX_STRINGS : 255                                */
/*    NETSTRING_USE_FILES   : defined                            */
/*    NETSTRING_USE_STREAMS : undefined (overrides USE_FILES)    */
/*    NETSTRING_FOR_SCGI    : undefined                          */
/*                                                               */
/*    In additional to the command line args options, the        */
/*    following environment variables can be used to change      */
/*    the way the netstring code handles the initial read        */
/*    of a netstring.  These variables exist mostly to affect    */
/*    the performance of the netstring library.                  */
/*                                                               */
/*    NETSTRING_INIT_READ_PREAMBLE : undefined                   */
/*    NETSTRING_INIT_READ_BRUTE    : undefined                   */
/*    NETSTRING_INIT_READ_MIN_SIZE : defined (DEFAULT)           */
/*                                                               */
/*****************************************************************/



#define TRUE  (0)
#define FALSE (!TRUE)

#define NETSTRING_ENCODE (0)
#define NETSTRING_DECODE (1)
#define NETSTRING_ERROR  (3)

#define NETSTRING_USE_FILES   (0)
#define NETSTRING_USE_STREAMS (1)

static  int operation    = NETSTRING_ENCODE;
static  int io_mechanism = NETSTRING_USE_FILES;
static  int scgi_mode    = FALSE;
static  int timing       = 0;

static void usage() {
   fprintf(stderr, "Invalid command line composition\n");
   exit(1);
}

/* COMMAND LINE OPTIONS */
static char *optstring = "ed";
static struct option longopts[] = {
  { "encode",      no_argument,        NULL,    'e' },
  { "decode",      no_argument,        NULL,    'd' },
  { "min-length",  required_argument,  NULL,    'm' },
  { "max-length",  required_argument,  NULL,    'M' },
  { "max-strings", required_argument,  NULL,    'a' },
  { "use-files",   no_argument,        NULL,    'f' },
  { "use-streams", no_argument,        NULL,    's' },
  { "scgi",        no_argument,        NULL,    'S' },
  { "timing",      optional_argument,  NULL,    't' },
  { NULL,          0,                  NULL,      0 }
};

static int read_options(int argc, char *argv[]) {
  char ch;  // Flag for command line option
  while ((ch = getopt_long(argc, argv, "ed", longopts, NULL)) != -1) {
    switch (ch) {
      case 'e' : // --encode
        operation = NETSTRING_ENCODE; break;
      case 'd' : // --decode
        operation = NETSTRING_DECODE; break;        
      case 'm' : // --min-length
        netstring_set_min_length(atoi(optarg)); break;
      case 'M' : // --max-length
        netstring_set_max_length(atoi(optarg)); break;
      case 'a' : // --max-strings <- array
        netstring_set_max_strings(atoi(optarg)); break;
      case 'f' : // --use-files
        io_mechanism = NETSTRING_USE_FILES; break;
      case 's' : // --use-streams
        io_mechanism = NETSTRING_USE_STREAMS; break;
      case 'S' : // --scgi
        scgi_mode=TRUE; break;
      case 't' : // --timing
        timing = (optarg == NULL) ? 0xFFFFF : atoi(optarg); break;
      default:
        usage();
    }
  }
  return optind;
}


static void set_options_via_envs() {
  char *value;
  value = getenv("NETSTRING_MIN_LENGTH");
  if (value != NULL) {
    netstring_set_min_length(atoi(value));
  }
  value = getenv("NETSTRING_MAX_LENGTH");
  if (value != NULL) {
    netstring_set_max_length(atoi(value));
  }
  value = getenv("NETSTRING_MAX_STRINGS");
  if (value != NULL) {
    netstring_set_max_strings(atoi(value));
  }
  value = getenv("NETSTRING_USE_FILES");
  if (value != NULL) {
    io_mechanism = NETSTRING_USE_FILES;
  }
  value = getenv("NETSTRING_USE_STREAMS");
  if (value != NULL) {
    io_mechanism = NETSTRING_USE_STREAMS;
  }

  value = getenv("NETSTRING_USE_SCGI");
  if (value != NULL) {
    netstring_set_max_strings(NETSTRING_PREAMBLE_MAX);
    scgi_mode = TRUE;
  }
}

static void set_init_read_mode(){
  char *value;

  value = getenv("NETSTRING_INIT_READ_PREAMBLE");
  if (value != NULL) {
    netstring_set_init_read(NETSTRING_INIT_READ_PREAMBLE);
  }
  value = getenv("NETSTRING_INIT_READ_BRUTE");
  if (value != NULL) {
    netstring_set_init_read(NETSTRING_INIT_READ_BRUTE);
  }
  value = getenv("NETSTRING_INIT_READ_MIN_SIZE");
  if (value != NULL) {
    netstring_set_init_read(NETSTRING_INIT_READ_MIN_SIZE);
  }

  return;
}


int main(int argc, char *argv[], char **envp) {

  int retval;

  int fd_in    = STDIN_FILENO;
  FILE *fp_in  = stdin;

  int fd_out   = STDOUT_FILENO;
  FILE *fp_out = stdout;


  // Set Options via ENVs
  set_options_via_envs();
  set_init_read_mode();

  // Read Command Line Options:
  int num_options = read_options(argc, argv);
  argc -= num_options;
  argv += num_options;

  // Handle FILE Names
  switch (argc) {
    case 2:
      fp_out = fopen(argv[1], "w");
      if (fp_out == NULL) {
        fprintf(stderr, "Invalid file for output \"%s\"", argv[1]);
      }
      fd_out = fileno(fp_out);
      // merge;

    case 1:
      fp_in = fopen(argv[0], "r");
      if (fp_out == NULL) {
        fprintf(stderr, "Invalid file for input \"%s\"", argv[1]);
      }

      fd_in = fileno(fp_in);
      break;

    case 0:
       break;

    default:
      usage();

  }

  switch (operation) {
    case NETSTRING_ENCODE:
      {
        NETSTRING *ns_p;
        char      *str;
        size_t    length;

        ns_p = netstring_start(0,0);

        str = fgetln(fp_in, &length);
        while (length != 0) {

          // remove the '\n' delimiter
          if (str[length - 1] == '\n') {
             str[length - 1] = '\0';
             length --;
          } 

          netstring_append(ns_p, str, length);
          str = fgetln(fp_in, &length);
        }

        netstring_end(ns_p);

        if (io_mechanism == NETSTRING_USE_STREAMS) {
          netstring_fwrite(ns_p, fp_out); // Using layer 3 for I/O
        } else {
          netstring_write(fd_out, ns_p);  // Using layer 2 for I/O
        }

        netstring_free(ns_p);
        break;
      }

    case NETSTRING_DECODE:
      { 
        NETSTRING *ns_p;
        char      **strings;
        int       ret_val;

        ns_p = netstring_allocate(0,0);

        if (io_mechanism == NETSTRING_USE_STREAMS) {
          for (int i=0; i <= timing ; i++) {
            if (i != 0) { rewind(fp_in); }
            retval = netstring_fread(ns_p, fp_in);
          }
        } else {
          for (int i=0; i <= timing ; i++) {
            if (timing != 0 ) { lseek(fd_in, 0,  SEEK_SET); }
            retval = netstring_read(fd_in, ns_p);
          }
        }
        if (retval != 0) {
          exit(retval);
        }
        netstring_end(ns_p);

        // env_p = netstring_strings(ns_p);
        strings = ns_p->strings;          // need an netstring access method
        //netstring_get_strings(ns_p);


        if (scgi_mode == TRUE) {
          retval = scgi_netstring_validate(ns_p);  // ensures the netstring conforms to the scgi protocol
          if (retval != 0) {
            exit(retval);
          }

          strings = scgi_netstring2env(ns_p);     // collapse (name, value) pairs into name=value strings
        }

        for(int count=0; strings[count] != NULL; count++) {
           fprintf(fp_out, "%s\n", strings[count]);
        }

        netstring_free(ns_p);
        break;

      default:
        assert(TRUE);
        break;
    }
  }

  return 0;

}
