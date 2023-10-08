#include <plugin_base/shared/utility.hpp>
#include <chrono>

namespace plugin_base {

double
seconds_since_epoch()
{
  auto ticks = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(ticks).count() / 1000000000.0;
}

}