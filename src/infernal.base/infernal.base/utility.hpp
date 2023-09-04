#pragma once
#include <cassert>

#define INF_STR_(x) #x
#define INF_STR(x) INF_STR_(x)
#define INF_VERSION_TEXT(major, minor) INF_STR(major.minor)

#define INF_ASSERT_EXEC(x) \
  do { if(!(x)) assert(false); } while(false)

#define INF_DECLARE_MOVE_ONLY(x) \
  x() = default; \
  x(x&&) = default; \
  x(x const&) = delete; \
  x& operator = (x&&) = default; \
  x& operator = (x const&) = delete
