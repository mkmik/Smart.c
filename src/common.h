// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#ifndef NS_COMMON_HEADER_DEFINED
#define NS_COMMON_HEADER_DEFINED

#include "net_skeleton.h"

const char *ns_skip(const char *, const char *, const char *, struct ns_str *);
int ns_ncasecmp(const char *s1, const char *s2, size_t len);

#endif  // NS_COMMON_HEADER_DEFINED
