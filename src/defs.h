#ifndef DEFS_H
#define DEFS_H

typedef enum {
  HTTP_GET
} HTTP_request_method;

typedef enum {
  HTTP_V09
} HTTP_version;

#define REQUEST_URI_LEN 256

typedef struct {
  HTTP_request_method method;
  char request_uri[REQUEST_URI_LEN];
  HTTP_version version;
} HTTP_request;

typedef enum {
  PARSING_SUCCESS,
  PARSING_ERROR,
} parsing_status;

typedef struct {
  parsing_status result;
  HTTP_request req;
} HTTP_parsed_request;

#endif // DEFS_H
