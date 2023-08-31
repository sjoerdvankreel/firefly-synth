#include <infernal.base/plugin_support.hpp>
#include <cmath>
#include <cstdint>

namespace infernal::base {

int 
hash(char const* text)
{
  std::uint32_t h = 0;
  int const multiplier = 33;
  auto utext = reinterpret_cast<std::uint8_t const*>(text);
  for (auto const* p = utext; *p != '\0'; p++) h = multiplier * h + *p;
  return std::abs(static_cast<int>(h + (h >> 5)));
}

}