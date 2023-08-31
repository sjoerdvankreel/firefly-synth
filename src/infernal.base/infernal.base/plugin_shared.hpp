#pragma once

#define INF_DECLARE_MOVE_ONLY(x) \
  x() = default; \
  x(x&&) = default; \
  x(x const&) = delete; \
  x& operator = (x&&) = default; \
  x& operator = (x const&) = delete

namespace infernal::base {

int hash(char const* text);
struct param_value final { union { float real; int step; }; };

}
