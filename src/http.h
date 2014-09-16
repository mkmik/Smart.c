// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#ifndef NS_HTTP_HEADER_DEFINED
#define NS_HTTP_HEADER_DEFINED

#include "common.h"

struct parsed_http_request {
  struct vec method;
  struct vec uri;
  struct vec proto;
  struct vec names[40];       // HTTP headers: names and values
  struct vec values[40];
};

int parse_http_request(const char *, int, struct parsed_http_request *);

enum {
  NS_HTTP_REQUEST
};

#endif  // NS_HTTP_HEADER_DEFINED