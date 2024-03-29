#include <plugin_base/dsp/utility.hpp>

// TODO figure out DAZ/FTZ equivalent for apple silicon
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace plugin_base {

std::pair<std::uint32_t, std::uint32_t>
disable_denormals()
{
#if defined(__i386__) || defined(__x86_64__)
  uint32_t ftz = _MM_GET_FLUSH_ZERO_MODE();
  uint32_t daz = _MM_GET_DENORMALS_ZERO_MODE();
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  return std::make_pair(ftz, daz);
#else
  return {};
#endif
}

void
restore_denormals(std::pair<std::uint32_t, std::uint32_t> state)
{
#if defined(__i386__) || defined(__x86_64__)
  _MM_SET_FLUSH_ZERO_MODE(state.first);
  _MM_SET_DENORMALS_ZERO_MODE(state.second);
#endif
}

}
