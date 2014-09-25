#include "../src/util.c"
#include "../src/http.c"
#include "../src/websocket.c"

#define FAIL(str, line) do {                    \
  printf("Fail on line %d: [%s]\n", line, str); \
  abort();                                      \
  return str;                                   \
} while (0)

#define ASSERT(expr) do {               \
  s_num_tests++;                        \
  if (!(expr)) FAIL(#expr, __LINE__);   \
} while (0)

#define RUN_TEST(test) do { const char *msg = test(); \
  if (msg) return msg; } while (0)

static int s_num_tests = 0;

static const char *test_parse_http_message(void) {
  static const char *a = "GET / HTTP/1.0\n\n";
  static const char *b = "GET /blah HTTP/1.0\r\nFoo:  bar  \r\n\r\n";
  static const char *c = "a b c\nz:  k \nb: t\nvvv\n\n xx";
  static const char *d = "a b c\nContent-Length: 21 \nb: t\nvvv\n\n";
  struct ns_str *v;
  struct http_message req;

  ASSERT(parse_http("\b23", 3, &req) == -1);
  ASSERT(parse_http("get\n\n", 5, &req) == -1);
  ASSERT(parse_http(a, strlen(a) - 1, &req) == 0);
  ASSERT(parse_http(a, strlen(a), &req) == (int) strlen(a));
  ASSERT(parse_http(b, strlen(b), &req) == (int) strlen(b));
  ASSERT(req.header_names[0].len == 3);
  ASSERT(req.header_values[0].len == 3);
  ASSERT(req.header_names[1].p == NULL);
  ASSERT(parse_http(c, strlen(c), &req) == (int) strlen(c) - 3);
  ASSERT(req.header_names[2].p == NULL);
  ASSERT(req.header_names[0].p != NULL);
  ASSERT(req.header_names[1].p != NULL);
  ASSERT(memcmp(req.header_values[1].p, "t", 1) == 0);
  ASSERT(req.header_names[1].len == 1);
  ASSERT(parse_http(d, strlen(d), &req) == (int) strlen(d));
  ASSERT(req.body.len == 21);
  ASSERT(req.message.len == 21 + strlen(d));
  ASSERT(get_http_header(&req, "foo") == NULL);
  ASSERT((v = get_http_header(&req, "contENT-Length")) != NULL);
  ASSERT(v->len == 2 && memcmp(v->p, "21", 2) == 0);

  return NULL;
}

static void cb1(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  if (ev == NS_HTTP_REQUEST) {
    ns_printf(nc, "HTTP/1.0 200 OK\n\n[%.*s %d]",
              (int) hm->uri.len, hm->uri.p, (int) hm->body.len);
    nc->flags |= NSF_FINISHED_SENDING_DATA;
  }
}

static void cb2(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  if (ev == NS_HTTP_REPLY) {
    memcpy(nc->user_data, hm->body.p, hm->body.len);
    nc->flags |= NSF_CLOSE_IMMEDIATELY;
  }
}

static const char *test_bind_http(void) {
  static const char *addr = "127.0.0.1:7777";
  struct ns_mgr mgr;
  struct ns_connection *nc, *nc2;
  char buf[20] = "";

  ns_mgr_init(&mgr, NULL);
  ASSERT(ns_bind_http(&mgr, addr, cb1, NULL) != NULL);

  // Valid HTTP request. Pass test buffer to the callback.
  ASSERT((nc = ns_connect_http(&mgr, addr, cb2, buf)) != NULL);
  ns_printf(nc, "%s", "POST /foo HTTP/1.0\nContent-Length: 10\n\n"
            "0123456789");

  // Invalid HTTP request
  ASSERT((nc2 = ns_connect(&mgr, addr, cb2, NULL)) != NULL);
  ns_printf(nc2, "%s", "bl\x03\n\n");

  { int i; for (i = 0; i < 50; i++) ns_mgr_poll(&mgr, 1); }
  ns_mgr_free(&mgr);

  // Check that test buffer has been filled by the callback properly.
  ASSERT(strcmp(buf, "[/foo 10]") == 0);

  return NULL;
}

static const char *run_all_tests(void) {
  RUN_TEST(test_parse_http_message);
  RUN_TEST(test_bind_http);
  return NULL;
}

int main(void) {
  const char *fail_msg = run_all_tests();
  printf("%s, tests run: %d\n", fail_msg ? "FAIL" : "PASS", s_num_tests);
  return fail_msg == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
