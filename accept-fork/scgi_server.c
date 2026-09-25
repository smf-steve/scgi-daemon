#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>

//extern int socket(int domain, int type, int protocol);
//extern int bind(int socket, const struct sockaddr *address, socklen_t address_len);
//extern int listen(int socket, int backlog);
//extern int accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);


#define PORT_NUM 1234

int main(int arc, char *argv[], char **envp) {

  int retval = 0;

  // Socket Related
  struct sockaddr_in    socket_in;
  int        any_addr = INADDR_ANY; 
  socklen_t  len = sizeof(struct sockaddr);

  int socket_fd;
  int connection_fd;

  // 
  int pid;

  int reason;
  int status;

  // Setup Server-Side Socket Info (input)
  memset( &socket_in, 0, len);
  socket_in.sin_family = PF_INET;
  socket_in.sin_port   = htons(PORT_NUM);
  memcpy( &socket_in.sin_addr, &any_addr, sizeof(long));

  // Allocate, Bind and Listen on the socket
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  retval = bind(socket_fd, (struct sockaddr *) &socket_in, len);
    if (retval != 0) perror("bind");
  retval = listen(socket_fd, 0);
    if (retval != 0) perror("listen");

  do { 
    // Accept a connection
    connection_fd = accept(socket_fd, (struct sockaddr *) &socket_in, &len);

    // fork a child to handle the request
    pid = fork();
    if (pid == 0) {
      retval = dup2(connection_fd,0);
      execle("/bin/cat", "cat", NULL, envp);
      fprintf(stderr, "Child process did not exec correctly\n");
    } 

    close(connection_fd);
    status = waitpid(pid, &reason, 0);
    fprintf(stdout, "\n\n----\nChild [%d] completed\n", pid);
  } while (0 == 0);
  return 0;
}
