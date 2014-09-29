// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved
//
// This program listens for the mpjpg stream over websocket, and streams it
// to the connected HTTP clients.

#include "smart.h"

static int s_received_signal = 0;
static const char *s_web_root = "./web_root";

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);
  s_received_signal = sig_num;
}

static void push_frame_to_clients(struct ns_mgr *mgr,
                                  const struct websocket_message *wm) {
  struct ns_connection *nc;
  for (nc = ns_next(mgr, NULL); nc != NULL; nc = ns_next(mgr, nc)) {
    if (!(nc->flags & NSF_USER_2)) continue;  // Ignore un-marked requests
    ns_printf(nc, "--w00t\r\nContent-Type: image/jpeg\r\n"
              "Content-Length: %lu\r\n\r\n", (unsigned long) wm->size);
    ns_send(nc, wm->data, wm->size);
    ns_send(nc, "\r\n", 2);
    printf("Image pushed to %p\n", nc);
  }
}

static void send_command_to_the_device(struct ns_mgr *mgr,
                                       const struct ns_str *cmd) {
  struct ns_connection *nc;
  for (nc = ns_next(mgr, NULL); nc != NULL; nc = ns_next(mgr, nc)) {
    if (!(nc->flags & NSF_USER_1)) continue;  // Ignore non-websocket requests
    ns_send_websocket(nc, WEBSOCKET_OP_TEXT, cmd->p, cmd->len);
    printf("Sent API command [%.*s] to %p\n", (int) cmd->len, cmd->p, nc);
  }
}

static void cb(struct ns_connection *nc, int ev, void *ev_data) {
  struct websocket_message *wm = (struct websocket_message *) ev_data;
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case NS_HTTP_REQUEST:
      if (ns_vcmp(&hm->uri, "/mjpg") == 0) {
        nc->flags |= NSF_USER_2;   // Set a mark on image requests
        ns_printf(nc, "%s",
                "HTTP/1.0 200 OK\r\n"
                "Cache-Control: no-cache\r\n"
                "Pragma: no-cache\r\n"
                "Expires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
                "Connection: close\r\n"
                "Content-Type: multipart/x-mixed-replace; "
                "boundary=--w00t\r\n\r\n");
      } else if (ns_vcmp(&hm->uri, "/api") == 0 && hm->body.len > 0) {
        // RESTful API call. HTTP message body should be a JSON message.
        // We should parse it and take appropriate action.
        // In our case, simply forward that call to the device.
        printf("API CALL: [%.*s] [%.*s]\n", (int) hm->method.len, hm->method.p,
               (int) hm->body.len, hm->body.p);
        send_command_to_the_device(nc->mgr, &hm->body);
        ns_printf(nc, "HTTP/1.0 200 OK\nContent-Length: 0\n\n");
      } else {
        ns_serve_uri_from_fs(nc, &hm->uri, s_web_root);
      }
      break;
    case NS_WEBSOCKET_FRAME:
      printf("Got websocket frame, size %lu\n", (unsigned long) wm->size);
      push_frame_to_clients(nc->mgr, wm);
      break;
  }
}

int main(int argc, char *argv[]) {
  struct ns_mgr mgr;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <listening_addr>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGPIPE, SIG_IGN);

  printf("Listening on: [%s]\n", argv[1]);

  ns_mgr_init(&mgr, NULL);

  if (ns_bind_http(&mgr, argv[1], cb, NULL) == NULL) {
    fprintf(stderr, "Error binding to %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  while (s_received_signal == 0) {
    ns_mgr_poll(&mgr, 1000);
  }
  ns_mgr_free(&mgr);

  printf("Quitting on signal %d\n", s_received_signal);

  return EXIT_SUCCESS;
}