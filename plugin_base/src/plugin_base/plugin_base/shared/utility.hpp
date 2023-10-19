#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cassert>
#include <utility>
#include <cstdint>
#include <iterator>
#include <algorithm>

#define INF_PREVENT_ACCIDENTAL_COPY(x)  \
  x(x&&) = default;               \
  x& operator = (x&&) = default;  \
  explicit x(x const&) = default; \
  x& operator = (x const&) = delete
#define INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(x) \
  INF_PREVENT_ACCIDENTAL_COPY(x); \
  x() = default

#define INF_STR_(x) #x
#define INF_STR(x) INF_STR_(x)
#define INF_VERSION_TEXT(major, minor) INF_STR(major.minor)
#define INF_ASSERT_EXEC(x) do { if(!(x)) assert(false); } while(false)

namespace plugin_base {

template <class Exit>
class scope_guard {
  Exit _exit;

public:
  ~scope_guard() { _exit(); }
  scope_guard(Exit exit) : _exit(exit) {}

  scope_guard(scope_guard&) = delete;
  scope_guard(scope_guard const&) = delete;          
  scope_guard& operator = (scope_guard&&) = delete;
  scope_guard& operator = (scope_guard const&) = delete;
};

double seconds_since_epoch();
inline void debug_breakable() {};

template <class T> std::string 
to_8bit_string(T const* source)
{
  T c = *source;
  std::string result;
  while (c != static_cast<T>('\0'))
  {
    result += static_cast<char>(c);
    c = *++source;
  }
  return result;
}

template <class T>
void from_8bit_string(T* dest, int count, char const* source)
{
  memset(dest, 0, sizeof(*dest) * count);
  for (int i = 0; i < count - 1 && i < strlen(source); i++)
    dest[i] = source[i];
}

template <class T, int N>
void from_8bit_string(T(&dest)[N], char const* source)
{ from_8bit_string(dest, N, source); }

template <class T> std::vector<T> 
vector_explicit_copy(std::vector<T> const& in)
{
  std::vector<T> result;
  for (int i = 0; i < in.size(); i++)
    result.push_back(T(in[i]));
  return result;
}

template <class T, class Pred> std::vector<T> 
vector_filter(std::vector<T> const& in, Pred pred)
{
  std::vector<T> result;
  std::copy_if(in.begin(), in.end(), std::back_inserter(result), pred);
  return result;
}

template <class T> std::vector<T>
vector_join(std::vector<std::vector<T>> const& vs)
{
  std::vector<T> result;
  for (int i = 0; i < vs.size(); i++)
    std::copy(vs[i].begin(), vs[i].end(), std::back_inserter(result));
  return result;
}

template <class T, class Unary> auto 
vector_map(std::vector<T> const& in, Unary op) -> std::vector<decltype(op(in[0]))>
{
  std::vector<decltype(op(in[0]))> result(in.size(), decltype(op(in[0])) {});
  std::transform(in.begin(), in.end(), result.begin(), op);
  return result;
}

}
