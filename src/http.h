// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#ifndef NS_HTTP_HEADER_DEFINED
#define NS_HTTP_HEADER_DEFINED

#include "common.h"

struct http_request {
  int num_headers;
  int request_len;
  struct vec method;
  struct vec uri;
  struct vec proto;
  struct vec names[40];       // HTTP headers: names and values
  struct vec values[40];
};

int parse_http_request(const char *, int, struct http_request *);
struct ns_connection *ns_bind_http(struct ns_mgr *, const char *addr, void *p,
                                   ns_callback_t cb);

#define NS_HTTP_REQUEST ((enum ns_event) (NS_CLOSE + 100))

#endif  // NS_HTTP_HEADER_DEFINED
