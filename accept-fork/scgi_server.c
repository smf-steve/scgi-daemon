#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>

#include "../netstring/scgi_netstring.h"


#define TRUE (0)
#define FALSE (!(TRUE))

#define PORT_NUM 1234

#define bind_p(fd, addr, len) {    \
  int retval;                      \
  retval = bind(fd, addr, len);    \
  if (retval != 0) perror("bind"); \
}

#define listen_p(fd, len) {          \
  int retval;                        \
  retval = listen(fd, len);          \
  if (retval != 0) perror("listen"); \
}


static int strprefix(char *prefix, char * str) {
   // returns true if str is euqivalent to prefix....
   int c;
   for (c = 0; prefix[c] != '\0'; c++) {
     if (str[c] == '\0') break;
     if (prefix[c] != str[c]) break;
   }

   return (prefix[c] == '\0');
}
extern int parse_command_line(int argc, char **envp);


char * get_config_file(int argc, char **argv) {

  char *relative_file = NULL;

  switch (argc) {
    case 1:   // Command Line:  argv[0]
      relative_file = "conf";
      break;

    case 2:   // Command Line:   argv[0] --conf=filename
      if (strprefix("--conf=", argv[1])) {
        relative_file = index(argv[1], '=') + 1;
      }
      break;

    case 3:   // Command Line:   argv[0] --conf filename
      if (strcmp("--conf", argv[1]) == 0) {
        relative_file = argv[2];
      }
      break;

    default:
      break;
  }

  return relative_file;
}

void source_config_file(char *home, char * filename) {
  ////////////////////////////////////////////
  //   Source the Configuration File
  FILE *fp;
  char absolute_filename[FILENAME_MAX];
  char *buffer;
  size_t capacity;
  int  length;

  if (filename[0] != '/') {
    // the conf file is a relative file name
    strlcat(absolute_filename, home, FILENAME_MAX);
  }
  strlcat(absolute_filename, filename, FILENAME_MAX);

  fp = fopen(absolute_filename, "r");
  if (fp == NULL) {
    char str[FILENAME_MAX];
    sscanf(str, "Unable to open config file: %s\n", filename);
    perror(str);
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


#define MAX_ENVS (50)


// The following are variables related to options. There
// values are set via `process_cli_options` and used with `main`
char * server_ip;
char * document_root;
char * error_log;
int    foreground_flag;
int    port;
char * scriptname;
char * socket_file;
char * user;
int    verbose_flag;
int    workers;
int    help_flag;
int    version_flag;



int main(int argc, char *argv[], char **envp) {

  char scgi_home[FILENAME_MAX];
  int num_options;
  int retval = 0;
  char *config_file;

  // Socket Related
  struct sockaddr_in    socket_in;
  int        any_addr = INADDR_ANY; 
  socklen_t  len = sizeof(struct sockaddr);

  int socket_fd;
  int connection_fd;
  int pid;
  int reason;
  int status;


  ////////////////////////////////////////////
  // Determine the HOME for the SCGI Daemon
  {
    char *env_p;
    env_p = getenv("SCGI_HOME");

    if (env_p != NULL) {
      strlcpy(scgi_home, env_p, FILENAME_MAX);
    } else {
      env_p = getenv("HOME");
      assert(env_p != NULL);
      strlcpy(scgi_home, env_p, FILENAME_MAX);
      strlcat(scgi_home, "/.scgi", FILENAME_MAX);
    }
  }

  ////////////////////////////////////////////
  // COMMAND LINE and ENV PROCESSING
  // 1. special case for using the --conf command line option
  //    : scgi [ --conf[=file ]]
  //
  // 2. normal processing of the command line
  //    : scgi [options] [ --port=<number> | --socket=<filename> ] [ exec ]
  //
  // 3. special case with use of --version and --help
  //    : scgi [--version | --help ]
  //
  // 4. check if there is only one parameter left ( [ exec ] 
  //

  // 1. Get the name of the config file from the command line, and source it
  config_file = get_config_file(argc, argv);
  if (config_file != NULL) {
    source_config_file(scgi_home, config_file);
    argc = 0;
    argv[0] = NULL;
  }


  // 2. Process the Command Line Options, which include environment variables
  num_options = parse_command_line(argc, argv);
  

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
      break;

    default:
      fprintf(stderr, "Superfluous command line arguments encourtered\n");
      exit(1);
  }
  ////////////////////////////////////////////




  // Setup Server-Side Socket Info (input)
  memset( &socket_in, 0, len);
  socket_in.sin_family = PF_INET;
  socket_in.sin_port   = htons(PORT_NUM);

  if (server_ip != NULL) {
    inet_pton(AF_INET, server_ip, (void *) &socket_in.sin_addr);
  } else {
    memcpy( &socket_in.sin_addr, &any_addr, sizeof(long));
  }

  // Allocate, Bind and Listen on the socket
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  bind_p(socket_fd, (struct sockaddr *) &socket_in, len);
  listen_p(socket_fd, 0);


  do { 
    // Accept a connection
    connection_fd = accept(socket_fd, (struct sockaddr *) &socket_in, &len);

    // fork a child to handle the request
    pid = fork();
    if (pid == 0) {
      // CHILD

      char *envp[MAX_ENVS];

      // Accept
      retval = dup2(connection_fd, STDIN_FILENO);
      retval = dup2(connection_fd, STDOUT_FILENO);

      scgi_read2env(STDIN_FILENO, envp, MAX_ENVS);
      // Do I need to add anything to the envp list

      // File Processing

      execle("/bin/cat", "cat", NULL, envp);

      scgi_free_env_buffer(envp);
      fprintf(stderr, "Child process did not exec correctly\n");
    } else {
      // PARENT
      close(connection_fd);
      status = waitpid(pid, &reason, 0);
      fprintf(stdout, "\n\n----\nChild [%d] completed\n", pid);
    }
  } while (TRUE);
  return 0;
}
