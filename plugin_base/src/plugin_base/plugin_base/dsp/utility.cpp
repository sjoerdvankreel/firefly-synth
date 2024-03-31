#include <plugin_base/dsp/utility.hpp>

// Note: sse2neon provides adapter macros to toggle the FPCR like it's done with immintrin.h.
#ifdef __aarch64__
#include <sse2neon.h>
#else
#include <immintrin.h>
#endif

namespace plugin_base {

std::pair<std::uint32_t, std::uint32_t>
disable_denormals()
{
  uint32_t ftz = _MM_GET_FLUSH_ZERO_MODE();
  uint32_t daz = _MM_GET_DENORMALS_ZERO_MODE();
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  return std::make_pair(ftz, daz);
}

void
restore_denormals(std::pair<std::uint32_t, std::uint32_t> state)
{
  _MM_SET_FLUSH_ZERO_MODE(state.first);
  _MM_SET_DENORMALS_ZERO_MODE(state.second);
}

}
