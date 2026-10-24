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

#include "scgi_netstring.h"
#include "scanopt.h"

#include "command_line.h"
extern void set_command_line_options(int argc, char *argv[]);


#define PID_FILE      "pid"
#define ERROR_FILE    "error.log"
#define DOCUMENT_DIR  "cgi"
#define SOCKET_FILE   "socket"

#define MAX_ENVS (50)


extern void set_command_line_options(int argc, char *argv[]);


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

#define freopen_p(filename, mode, fp) {       \
  FILE * fp;                                  \
  fp = freopen(ERROR_FILE, "w", stderr);      \
  if (fp == 0)                                \
    perror("Unable to open error logfile\n"); \
}


int main(int argc, char *argv[], char **envp) {

  char *document_home = DOCUMENT_DIR;

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

  set_command_line_options(argc, argv);

  if (foreground_flag == false) {
    pid = fork();
    if (pid != 0) {
      // child
      // daemonize();
    } else {
      exit(1);
    }
  }

  chdir(scgi_dir);

  freopen_p(ERROR_FILE, "w", stderr);

   

  // Setup the Socket
  if (port != 0) {
    // Setup Server-Side Socket Info (input)
    memset( &socket_in, 0, len);
    socket_in.sin_family = PF_INET;
    socket_in.sin_port   = htons(port);

    if (server_ip != NULL) {
      inet_pton(AF_INET, server_ip, (void *) &socket_in.sin_addr);
    } else {
      memcpy( &socket_in.sin_addr, &any_addr, sizeof(long));
    }

    // Allocate, Bind and Listen on the socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  }
  if (socket_flag == SET) {
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

      /////////////////////////////////////////////////////////
      // File Processing





      /////////////////////////////////////////////////////////
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

