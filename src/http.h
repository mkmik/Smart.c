// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#ifndef NS_HTTP_HEADER_DEFINED
#define NS_HTTP_HEADER_DEFINED

#include "common.h"

#define NS_MAX_HTTP_HEADERS 40

struct http_request {
  struct vec message;         // Whole message: request line + headers + body

  // HTTP Request line
  struct vec method;          // "GET"
  struct vec uri;             // "/my_file.html"
  struct vec proto;           // "HTTP/1.1"

  // Headers
  struct vec header_names[NS_MAX_HTTP_HEADERS];
  struct vec header_values[NS_MAX_HTTP_HEADERS];

  // Message body
  struct vec body;            // Zero-length for requests with no body
};

//int parse_http_request(const char *, int, struct http_request *);

typedef void (*http_callback_t)(struct ns_connection *,
                                const struct http_request *);

struct ns_connection *ns_bind_http(struct ns_mgr *mgr,
                                   const char *addr,
                                   void *user_data,
                                   http_callback_t http_request_callback);

struct vec *get_http_header(struct http_request *, const char *);

#define NS_HTTP_REQUEST ((enum ns_event) (NS_CLOSE + 100))
#define NS_WEBSOCKET_HANDSHAKE_REQUEST ((enum ns_event) (NS_CLOSE + 110))
#define NS_WEBSOCKET_HANDSHAKE_COMPLETE ((enum ns_event) (NS_CLOSE + 111))
#define NS_WEBSOCKET_FRAME ((enum ns_event) (NS_CLOSE + 112))

// Websocket opcodes, from http://tools.ietf.org/html/rfc6455
enum {
  WEBSOCKET_OPCODE_CONTINUATION = 0x0,
  WEBSOCKET_OPCODE_TEXT = 0x1,
  WEBSOCKET_OPCODE_BINARY = 0x2,
  WEBSOCKET_OPCODE_CONNECTION_CLOSE = 0x8,
  WEBSOCKET_OPCODE_PING = 0x9,
  WEBSOCKET_OPCODE_PONG = 0xa
};

#endif  // NS_HTTP_HEADER_DEFINED
