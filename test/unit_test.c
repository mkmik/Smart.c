#include "../src/http.c"

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
  struct http_request req;

  ASSERT(parse_http_request("\b23", 3, &req) == -1);
  ASSERT(parse_http_request("get\n\n", 5, &req) == -1);
  ASSERT(parse_http_request(a, strlen(a) - 1, &req) == 0);
  ASSERT(parse_http_request(a, strlen(a), &req) == (int) strlen(a));
  ASSERT(parse_http_request(b, strlen(b), &req) == (int) strlen(b));
  ASSERT(req.names[0].len == 3);
  ASSERT(req.values[0].len == 3);
  ASSERT(req.names[1].p == NULL);
  ASSERT(parse_http_request(c, strlen(c), &req) == (int) strlen(c) - 3);
  ASSERT(req.names[2].p == NULL);
  ASSERT(req.names[0].p != NULL);
  ASSERT(req.names[1].p != NULL);
  ASSERT(memcmp(req.values[1].p, "t", 1) == 0);
  ASSERT(req.names[1].len == 1);

  return NULL;
}

static void cb1(struct ns_connection *nc, enum ns_event ev, void *p) {
  struct http_request *req = (struct http_request *) p;

  switch (ev) {
    case NS_HTTP_REQUEST:
      ns_printf(nc, "HTTP/1.0 200 OK\n\n[%.*s]", (int) req->uri.len, req->uri.p);
      nc->flags |= NSF_FINISHED_SENDING_DATA;
      break;
    default:
      break;
  }
}

static const char *test_bind_http(void) {
  static const char *addr = "127.0.0.1:7777";
  struct ns_mgr mgr;
  struct ns_connection *nc, *nc2;

  ns_mgr_init(&mgr, NULL, NULL);
  ASSERT(ns_bind_http(&mgr, addr, NULL, cb1) != NULL);

  // Valid HTTP request
  ASSERT((nc = ns_connect(&mgr, addr, NULL)) != NULL);
  ns_printf(nc, "%s", "GET /foo HTTP/1.0\n\n");

  // Invalid HTTP request
  ASSERT((nc2 = ns_connect(&mgr, addr, NULL)) != NULL);
  ns_printf(nc2, "%s", "bl\x03\n\n");

  { int i; for (i = 0; i < 50; i++) ns_mgr_poll(&mgr, 1); }
  ns_mgr_free(&mgr);

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
