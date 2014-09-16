// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#include <ctype.h>
#include "http.h"

static const char *skip(const char *s, const char *end,
                        const char *delims, struct vec *v) {
  v->p = s;
  while (s < end && strchr(delims, * (unsigned char *) s) == NULL) s++;
  v->len = s - v->p;
  while (s < end && strchr(delims, * (unsigned char *) s) != NULL) s++;
  return s;
}

// Check whether full request is buffered. Return:
//   -1  if request is malformed
//    0  if request is not yet fully buffered
//   >0  actual request length, including last \r\n\r\n
static int get_request_len(const char *s, int buf_len) {
  const unsigned char *buf = (unsigned char *) s;
  int i;

  for (i = 0; i < buf_len; i++) {
    // Control characters are not allowed but >=128 are.
    // Abort scan as soon as one malformed character is found.
    if (!isprint(buf[i]) && buf[i] != '\r' && buf[i] != '\n' && buf[i] < 128) {
      return -1;
    } else if (buf[i] == '\n' && i + 1 < buf_len && buf[i + 1] == '\n') {
      return i + 2;
    } else if (buf[i] == '\n' && i + 2 < buf_len && buf[i + 1] == '\r' &&
               buf[i + 2] == '\n') {
      return i + 3;
    }
  }

  return 0;
}

int parse_http_request(const char *s, int n, struct parsed_http_request *req) {
  const char *end;
  int len, num_headers = 0;

  if ((len = get_request_len(s, n)) <= 0) return len;
  end = s + len;

  // Request is fully buffered. Skip leading whitespaces.
  while (s < end && isspace(* (unsigned char *) s)) s++;

  // Parse request line: method, URI, proto
  s = skip(s, end, " ", &req->method);
  s = skip(s, end, " ", &req->uri);
  s = skip(s, end, "\r\n", &req->proto);
  if (req->uri.p <= req->method.p || req->proto.p <= req->uri.p) return -1;

  while (num_headers < (int) (sizeof(req->names) / sizeof(req->names[0]))) {
    struct vec *v = &req->values[num_headers];

    s = skip(s, end, ": ", &req->names[num_headers]);
    s = skip(s, end, "\r\n", &req->values[num_headers]);

    while (v->len > 0 && v->p[v->len - 1] == ' ') {
      v->len--;  // Trim trailing spaces in header value
    }

    if (req->names[num_headers].len == 0 || req->values[num_headers].len == 0) {
      req->names[num_headers].p = req->values[num_headers].p = NULL;
      break;
    }
    num_headers++;
  }

  return len;
}