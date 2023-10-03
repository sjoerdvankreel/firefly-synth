#include <plugin_base/utility.hpp>

#include <chrono>
#include <immintrin.h>

namespace plugin_base {

std::pair<uint32_t, uint32_t> 
disable_denormals()
{
  uint32_t ftz = _MM_GET_FLUSH_ZERO_MODE();
  uint32_t daz = _MM_GET_DENORMALS_ZERO_MODE();
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  return std::make_pair(ftz, daz);
}

void
restore_denormals(std::pair<uint32_t, uint32_t> state)
{
  _MM_SET_FLUSH_ZERO_MODE(state.first);
  _MM_SET_DENORMALS_ZERO_MODE(state.second);
}

double
seconds_since_epoch()
{
  auto ticks = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(ticks).count() / 1000000000.0;
}

}