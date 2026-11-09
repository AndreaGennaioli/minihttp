#include <errno.h>
#include <linux/openat2.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/syscall.h>
#include <signal.h>
#include <unistd.h>

#include "defs.h"

#define SENDFILE_CHUNK (1 << 20)   // 1 MiB (2^20) per call

int pase_port(const char *str);

int open_static_file(int root_fd, const char *path);

HTTP_parsed_request parse_request(char *buf, int size);

int main(int argc, char*argv[]) {
  int socket_fd, client_fd, port;
  int recv_len;
  int root_fd, file_fd;
  ssize_t nsent;
  char buf[4096], root_dir[] = "./test/www/";
  HTTP_parsed_request req;
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

  // open root dir for static file serving
  root_fd = open(root_dir,  O_RDONLY | O_DIRECTORY);
  if (root_fd < 0) {
    fprintf(stderr, "cannot open %s as static file root directory\n", root_dir);
    return 1;
  }

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket()");
 return 1;
  }

  // set SO_REUSEADDR so that bind() succeeds even if previous connections on this port
  // are still in TIME_WAIT.
  // useful to let the server be restarted on the same port immediately.
  int opt = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt()");
    close(socket_fd);
    return 1;
  }

  if (bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    perror("bind()");
    close(socket_fd);
    return 1;
  }

  if (listen(socket_fd, 5) < 0) {
    perror("listen()");
    close(socket_fd);
    return 1;
  }

  // explicitly set Ctrl+C to its default action. (It is already set to SIG_DFL)
  signal(SIGINT, SIG_DFL);
  // ignoring the signal in case the client closes the connection during write/read
  signal(SIGPIPE, SIG_IGN);

  while(1) {
    file_fd = -1;
    client_fd = accept(socket_fd, (struct sockaddr *)&clientaddr, &clientaddr_len);

    if (client_fd < 0) {
      perror("accept()");
      continue;
    }

    recv_len = recv(client_fd, buf, sizeof(buf), 0);
    if (recv_len < 0) {
      perror("recv()");
      goto skip_conn;
    }

    req = parse_request(buf, recv_len);
    if (req.result != PARSING_SUCCESS) {
      goto skip_conn;
    }

    file_fd = open_static_file(root_fd, req.req.request_uri);
    if (file_fd < 0) {
      perror("error opening static file");
      goto skip_conn;
    }

    while ((nsent = sendfile(client_fd, file_fd, NULL, SENDFILE_CHUNK)) > 0);
    if (nsent == -1) {
      perror("sendfile()");
    }

  skip_conn:
    if (file_fd != -1)
      close(file_fd);
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

int open_static_file(int root_fd, const char *path) {
  struct open_how oh = {0};
  oh.flags = O_RDONLY | OPENAT2_REGULAR;
  oh.resolve = RESOLVE_IN_ROOT | RESOLVE_NO_MAGICLINKS;
  int fd = syscall(SYS_openat2, root_fd, path, &oh, sizeof(struct open_how));
  if (fd < 0) {
    return -1;
  }
  return fd;
}

HTTP_parsed_request parse_request(char *buf, int size) {
  const char *uri, *end;
  int uri_len;
  HTTP_parsed_request result = {
    .result = PARSING_ERROR,
    .req = {
      .method = HTTP_GET,
      .request_uri = {0},
      .version = HTTP_V09
    }
  };

  // minimal valid message is "GET a\r\n"
  if (size < 7 || memcmp(buf, "GET ", 4) != 0) {
    return result;
  }

  uri = buf + 4;
  end = memchr(uri, '\r', size-4);
  if (end == NULL || end+2 != buf+size || end[1] != '\n') {
    return result;
  }

  uri_len = end - uri;
  // uri_len can be at max REQUEST_URI_LEN-1 long
  if (uri_len == 0 || uri_len >= REQUEST_URI_LEN) {
    return result;
  }

  memcpy(result.req.request_uri, uri, uri_len);
  result.req.request_uri[uri_len] = '\0';
  result.result = PARSING_SUCCESS;

  return result;
}
