// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#include "http.h"

// Check whether full request is buffered. Return:
//   -1  if request is malformed
//    0  if request is not yet fully buffered
//   >0  actual request length, including last \r\n\r\n
static int get_request_len(const char *s, int buf_len) {
  const unsigned char *buf = (unsigned char *) s;
  int i;

  for (i = 0; i < buf_len; i++) {
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

static int parse_http(const char *s, int n, struct http_message *req) {
  const char *end;
  int len, i;

  if ((len = get_request_len(s, n)) <= 0) return len;

  memset(req, 0, sizeof(*req));
  req->message.p = s;
  req->body.p = s + len;
  req->message.len = req->body.len = ~0;
  //req->body.len = ~0;
  end = s + len;

  // Request is fully buffered. Skip leading whitespaces.
  while (s < end && isspace(* (unsigned char *) s)) s++;

  // Parse request line: method, URI, proto
  s = ns_skip(s, end, " ", &req->method);
  s = ns_skip(s, end, " ", &req->uri);
  s = ns_skip(s, end, "\r\n", &req->proto);
  if (req->uri.p <= req->method.p || req->proto.p <= req->uri.p) return -1;

  for (i = 0; i < (int) ARRAY_SIZE(req->header_names); i++) {
    struct ns_str *k = &req->header_names[i], *v = &req->header_values[i];

    s = ns_skip(s, end, ": ", k);
    s = ns_skip(s, end, "\r\n", v);

    while (v->len > 0 && v->p[v->len - 1] == ' ') {
      v->len--;  // Trim trailing spaces in header value
    }

    if (k->len == 0 || v->len == 0) {
      k->p = v->p = NULL;
      break;
    }

    if (!ns_ncasecmp(k->p, "Content-Length", 14)) {
      req->body.len = strtoul(v->p, NULL, 10);
      req->message.len = len + req->body.len;
    }
  }

  return len;
}

struct ns_str *get_http_header(struct http_message *hm, const char *name) {
  size_t i, len = strlen(name);

  for (i = 0; i < ARRAY_SIZE(hm->header_names); i++) {
    struct ns_str *h = &hm->header_names[i], *v = &hm->header_values[i];
    if (h->p != NULL && h->len == len && !ns_ncasecmp(h->p, name, len)) return v;
  }

  return NULL;
}

static void http_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct iobuf *io = &nc->recv_iobuf;
  ns_callback_t cb = (ns_callback_t) nc->proto_data;
  struct http_message hm;
  int req_len;

  if (cb) cb(nc, ev, ev_data);

  switch (ev) {

    case NS_RECV:
      req_len = parse_http(io->buf, io->len, &hm);
      if (req_len < 0 || io->len >= NS_MAX_HTTP_REQUEST_SIZE) {
        nc->flags |= NSF_CLOSE_IMMEDIATELY;
      } else if (req_len > 0 && hm.message.len <= io->len) {
        if (cb) cb(nc, nc->listener ? NS_HTTP_REQUEST : NS_HTTP_REPLY, &hm);
        iobuf_remove(io, hm.message.len);
      }
      break;

    case NS_CLOSE:
      if (io->len > 0 && parse_http(io->buf, io->len, &hm) > 0 && cb) {
        hm.body.len = io->buf + io->len - hm.body.p;
        cb(nc, nc->listener ? NS_HTTP_REQUEST : NS_HTTP_REPLY, &hm);
      }
      break;

    case NS_CONNECT:
      if (* (int *) ev_data == 0) {
      }
      break;

    default:
      break;
  }
}

struct ns_connection *ns_bind_http(struct ns_mgr *mgr, const char *addr,
                                   ns_callback_t cb, void *user_data) {
  struct ns_connection *nc = ns_bind(mgr, addr, http_handler, user_data);
  if (nc != NULL) {
    nc->proto_data = (void *) cb;
  }
  return nc;
}

struct ns_connection *ns_connect_http(struct ns_mgr *mgr, const char *addr,
                                      ns_callback_t cb, void *user_data) {
  struct ns_connection *nc = ns_connect(mgr, addr, http_handler, user_data);

  if (nc != NULL) {
    nc->proto_data = (void *) cb;
  }
  return nc;
}
