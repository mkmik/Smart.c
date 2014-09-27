// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved
//
// This program polls given file, and if it is modified, it sends it
// over the websocket connection to the specified server.

#include "smart.h"

static int s_received_signal = 0;
static int s_connected = 0;

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);
  s_received_signal = sig_num;
}

static void send_mjpg_frame(struct ns_connection *nc, const char *file_path) {
  struct stat st;
  FILE *fp;

  // Check file modification time.
  // If changed, send file content to the websocket
  if (stat(file_path, &st) == 0 &&
      st.st_mtime != (time_t) nc->user_data &&
      (fp = fopen(file_path, "rb")) != NULL) {

    // Read new mjpg frame into a buffer
    char buf[st.st_size];
    fread(buf, 1, sizeof(buf), fp);  // TODO (lsm): check error
    fclose(fp);

    // Send that buffer to a websocket connection
    ns_send_websocket(nc, WEBSOCKET_OP_BINARY, buf, sizeof(buf));
    printf("Sent mjpg frame, %lu bytes\n", (unsigned long) sizeof(buf));

    // Store new modification time
    nc->user_data = (void *) st.st_mtime;
  }
}

static void cb(struct ns_connection *nc, int ev, void *ev_data) {
  (void) ev_data;
  switch (ev) {
    case NS_CONNECT:
      printf("Reconnect %s\n", * (int *) ev_data == 0 ? "ok" : "failed");
      break;
    case NS_CLOSE:
      printf("Connection %p closed\n", nc);
      s_connected = 0;
      break;
    case NS_POLL:
      send_mjpg_frame(nc, (char *) nc->mgr->user_data);
      break;
  }
}

int main(int argc, char *argv[]) {
  struct ns_mgr mgr;
  time_t last_reconnect_time = 0, now = 0;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <mjpg_file> <poll_interval_ms> "
            "<server_address>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGPIPE, SIG_IGN);

  printf("Streaming [%s] to [%s]\n", argv[1], argv[3]);

  ns_mgr_init(&mgr, argv[1]);

  while (s_received_signal == 0) {
    now = ns_mgr_poll(&mgr, atoi(argv[2]));
    if (s_connected == 0 && now - last_reconnect_time > 0) {
      // Reconnect if disconnected
      printf("Reconnecting to %s...\n", argv[3]);
      ns_connect_websocket(&mgr, argv[3], cb, NULL, "/stream", NULL);
      last_reconnect_time = now;  // Rate-limit reconnections to 1 per second
      s_connected = 1;
    }
  }
  ns_mgr_free(&mgr);

  printf("Quitting on signal %d\n", s_received_signal);

  return EXIT_SUCCESS;
}