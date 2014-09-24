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

static int lowercase(const char *s) {
  return tolower(* (const unsigned char *) s);
}

static int ncasecmp(const char *s1, const char *s2, size_t len) {
  int diff = 0;

  if (len > 0)
    do {
      diff = lowercase(s1++) - lowercase(s2++);
    } while (diff == 0 && s1[-1] != '\0' && --len > 0);

  return diff;
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

int parse_http_request(const char *s, int n, struct http_request *req) {
  const char *end;
  int len, i;

  if ((len = get_request_len(s, n)) <= 0) return len;

  memset(req, 0, sizeof(*req));
  req->message.p = s;
  req->message.len = len;
  req->body.p = s + len;
  end = s + len;

  // Request is fully buffered. Skip leading whitespaces.
  while (s < end && isspace(* (unsigned char *) s)) s++;

  // Parse request line: method, URI, proto
  s = skip(s, end, " ", &req->method);
  s = skip(s, end, " ", &req->uri);
  s = skip(s, end, "\r\n", &req->proto);
  if (req->uri.p <= req->method.p || req->proto.p <= req->uri.p) return -1;

  for (i = 0; i < (int) ARRAY_SIZE(req->header_names); i++) {
    struct vec *k = &req->header_names[i], *v = &req->header_values[i];

    s = skip(s, end, ": ", k);
    s = skip(s, end, "\r\n", v);

    while (v->len > 0 && v->p[v->len - 1] == ' ') {
      v->len--;  // Trim trailing spaces in header value
    }

    if (k->len == 0 || v->len == 0) {
      k->p = v->p = NULL;
      break;
    }

    if (!ncasecmp(k->p, "Content-Length", 14)) {
      req->body.len = strtoul(v->p, NULL, 10);
      req->message.len += req->body.len;
    }
  }

  return len;
}

struct vec *get_http_header(struct http_request *hr, const char *name) {
  size_t i, len = strlen(name);

  for (i = 0; i < ARRAY_SIZE(hr->header_names); i++) {
    struct vec *h = &hr->header_names[i], *v = &hr->header_values[i];
    if (h->p != NULL && h->len == len && !ncasecmp(h->p, name, len)) return v;
  }

  return NULL;
}

static void http_cb(struct ns_connection *nc, enum ns_event ev, void *p) {
  struct iobuf *io = &nc->recv_iobuf;
  struct http_request hr;
  int req_len;

  (void) p;

  switch (ev) {
    case NS_RECV:
      req_len = parse_http_request(io->buf, io->len, &hr);
      if (req_len < 0) {
        nc->flags |= NSF_CLOSE_IMMEDIATELY;
      } else if (req_len > 0 && hr.message.len <= io->len &&
                 nc->listener && nc->listener->proto_data) {
        ((http_callback_t) nc->listener->proto_data)(nc, &hr);
        iobuf_remove(io, hr.message.len);
      }
      break;
    default: break;
  }
}

struct ns_connection *ns_bind_http(struct ns_mgr *mgr, const char *addr,
                                   void *user_data, http_callback_t cb) {
  struct ns_connection *nc = ns_bind(mgr, addr, user_data);
  if (nc != NULL) {
    nc->callback = http_cb;
    nc->proto_data = (void *) cb;
  }
  return nc;
}
