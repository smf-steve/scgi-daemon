#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "../netstring/scgi_netstring.h"
#include "scanopt.h"
#include "command_line.h"

#define SET (1)
#define UNSET (0)


extern void set_command_line_options(int argc, char *argv[], char *home);


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



#define DEFAULT_CONFIG_FILE   "config"
#define DEFAULT_PID_FILE      "pid"
#define DEFAULT_ERROR_FILE    "error.log"
#define DEFAULT_DOCUMENT_FILE "cgi"
#define DEFAULT_SOCKET_FILE   "socket"

// What is the value of recording the children pid.
   // only out-of-bound -- cleanup, ???

// RESTRICTION: only one scgi_deamon per SCGI_HOME
//  SCGI_HOME=... scgi_daemon ....  



int main(int argc, char *argv[], char **envp) {

  char scgi_home[FILENAME_MAX];
  char working_directory[FILENAME_MAX];
  char config_file[FILENAME_MAX];
  char document_home[FILENAME_MAX];

  int retval = 0;

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
      strlcat(scgi_home, "/", FILENAME_MAX);
    } else {
      env_p = getenv("HOME");
      assert(env_p != NULL);
      strlcpy(scgi_home, env_p, FILENAME_MAX);
      strlcat(scgi_home, "/.scgi/", FILENAME_MAX);
    }
  }
  // set default values that might be overriden by the commandline:
  // these are the values that are related to scgi_home

  set_command_line_options(argc, argv, scgi_home);
  
  // set_command_line options
     // values that might be set
     // config
     // document_root
     // error_file
     // socket_file -- why does the user need to define the socket file
     //   -- they should only be to say use uds or inet socket

  chdir(scgi_home);




  //////////////////////////////////////////////////
  // Set things up.. Open files, etc.

  if (foreground_flag == false) {
    pid = fork();
    if (pid == 0) {
      // child
      // daemonize();
    } else {
      exit(1);
    }
  }


  // Setup the Socket
  if (port != NULL) {
    // Setup Server-Side Socket Info (input)
    memset( &socket_in, 0, len);
    socket_in.sin_family = PF_INET;
    socket_in.sin_port   = htons(port_num);

    if (server_ip != NULL) {
      inet_pton(AF_INET, server_ip, (void *) &socket_in.sin_addr);
    } else {
      memcpy( &socket_in.sin_addr, &any_addr, sizeof(long));
    }

    // Allocate, Bind and Listen on the socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  }
  if (socket_file != NULL) {
    fprintf(stderr, "socket file not implemented.\n");
    exit(1);
  }

  

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

      //scgi_read2env(STDIN_FILENO, envp, MAX_ENVS);
      // Do I need to add anything to the envp list

      // File Processing

      execle("/bin/cat", "cat", NULL, envp);

      //scgi_free_env_buffer(envp);
      fprintf(stderr, "Child process did not exec correctly\n");
    } else {
      // PARENT
      close(connection_fd);
      status = waitpid(pid, &reason, 0);
      fprintf(stdout, "\n\n----\nChild [%d] completed\n", pid);
    }
  } while (true);
  return 0;
}

