#pragma once
#include <cassert>

#define INF_ASSERT_EXEC(x) \
  do { if(!(x)) assert(false); } while(false)

#define INF_DECLARE_MOVE_ONLY(x) \
  x() = default; \
  x(x&&) = default; \
  x(x const&) = delete; \
  x& operator = (x&&) = default; \
  x& operator = (x const&) = delete
