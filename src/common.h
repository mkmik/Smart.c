// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved

#ifndef NS_COMMON_HEADER_DEFINED
#define NS_COMMON_HEADER_DEFINED

#include "net_skeleton.h"

// Describes some memory chunk
struct vec {
  const char *p;    // Points to the memory chunk
  size_t len;       // Size of memory chunk
};

#endif  // NS_COMMON_HEADER_DEFINED
