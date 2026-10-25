#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>

int pase_port(const char *str);

int main(int argc, char*argv[]) {
  int socket_fd, client_fd, port;
  int recv_len, sent_len;
  char buf[4096];
  struct sockaddr_in serveraddr = {
    .sin_family = AF_INET,
    .sin_port = 0,
    .sin_addr = INADDR_ANY
  };
  struct sockaddr_in clientaddr = {0};
  socklen_t clientaddr_len = sizeof(clientaddr);

  // invocation: mhttp PORT
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PORT\n", argv[0]);
    return 1;
  }

  port = pase_port(argv[1]);
  if (port < 0) {
    fprintf(stderr, "the provided port number is not valid: %s\n", argv[1]);
    return 1;
  }

  serveraddr.sin_port = htons((uint16_t) port);

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket()");
    return 1;
  }

  if (bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    perror("bind()");
    return 1;
  }

  if (listen(socket_fd, 5) < 0) {
    perror("listen()");
    return 1;
  }

  // explicitly set Ctrl+C to its default action. (It is already set to SIG_DFL)
  signal(SIGINT, SIG_DFL);

  while(1) {
    client_fd = accept(socket_fd, (struct sockaddr *)&clientaddr, &clientaddr_len);

    if (client_fd < 0) {
      perror("accept()");
      continue;
    }

    recv_len = recv(client_fd, buf, sizeof(buf), 0);
    if (recv_len < 0) {
      close(client_fd);
      perror("recv()");
      continue;
    }

    // MSG_NOSIGNAL prevents send() from raising SIGPIPE if the client closes the connection
    // before the response is sent
    sent_len = send(client_fd, buf, recv_len, MSG_NOSIGNAL);
    if (sent_len < 0) {
      perror("send()");
      // fall-through to close and repeat loop
    }

    close(client_fd);
  }

  // since the while(1) loop, for now this is dead code.
  close(socket_fd);
  return 0;
}

int pase_port(const char *str) {
  char *end;
  errno = 0;
  long n = strtol(str, &end, 10);
  if (errno || *end != '\0' || end == str || n < 1 || n > 65535) {
    return -1;
  }
  return n;
}
