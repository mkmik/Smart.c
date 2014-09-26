// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#ifndef NS_COMMON_HEADER_DEFINED
#define NS_COMMON_HEADER_DEFINED

#include "net_skeleton.h"
#include "sha1.h"

const char *ns_skip(const char *, const char *, const char *, struct ns_str *);
int ns_ncasecmp(const char *s1, const char *s2, size_t len);
void ns_base64_decode(const unsigned char *s, int len, char *dst);
void ns_base64_encode(const unsigned char *src, int src_len, char *dst);

#endif  // NS_COMMON_HEADER_DEFINED
