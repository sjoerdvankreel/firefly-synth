#ifndef INFERNAL_BASE_PLUGIN_SUPPORT_HPP
#define INFERNAL_BASE_PLUGIN_SUPPORT_HPP

#define INF_DECLARE_MOVE_ONLY(x) \
  x() = default; \
  x(x&&) = default; \
  x(x const&) = delete; \
  x& operator = (x&&) = default; \
  x& operator = (x const&) = delete

namespace infernal::base {

int stable_hash_nonnegative(char const* text);

}
#endif