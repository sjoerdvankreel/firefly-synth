#pragma once

#include <string>
#include <cstring>
#include <cassert>

#define INF_DECLARE_MOVE_ONLY(x) \
  x(x&&) = default;              \
  x(x const&) = delete;          \
  x& operator = (x&&) = default; \
  x& operator = (x const&) = delete
#define INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(x) \
  INF_DECLARE_MOVE_ONLY(x); \
  x() = default

#define INF_STR_(x) #x
#define INF_STR(x) INF_STR_(x)
#define INF_VERSION_TEXT(major, minor) INF_STR(major.minor)
#define INF_ASSERT_EXEC(x) do { if(!(x)) assert(false); } while(false)

namespace plugin_base {

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
{
  from_8bit_string(dest, N, source);
}

}
