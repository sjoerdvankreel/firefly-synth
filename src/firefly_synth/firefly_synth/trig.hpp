#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <cmath>

// trigonometric stuff for lfos and shapers
namespace firefly_synth
{

inline float sin_m11(float x) { return std::sin(x * plugin_base::pi32); }
inline float cos_m11(float x) { return std::cos(x * plugin_base::pi32); }
inline float sin_sin_m11(float x) { return std::sin(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32)); }
inline float sin_cos_m11(float x) { return std::sin(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32)); }
inline float cos_sin_m11(float x) { return std::cos(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32)); }
inline float cos_cos_m11(float x) { return std::cos(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32)); }
inline float sin_sin_sin_m11(float x) { return std::sin(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32))); }
inline float sin_sin_cos_m11(float x) { return std::sin(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32))); }
inline float sin_cos_sin_m11(float x) { return std::sin(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32))); }
inline float sin_cos_cos_m11(float x) { return std::sin(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32))); }
inline float cos_sin_sin_m11(float x) { return std::cos(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32))); }
inline float cos_sin_cos_m11(float x) { return std::cos(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32))); }
inline float cos_cos_sin_m11(float x) { return std::cos(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32 + std::sin(x * plugin_base::pi32))); }
inline float cos_cos_cos_m11(float x) { return std::cos(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32 + std::cos(x * plugin_base::pi32))); }

inline float sin_01(float x) { return std::sin(x * 2 * plugin_base::pi32); }
inline float cos_01(float x) { return std::cos(x * 2 * plugin_base::pi32); }
inline float sin_sin_01(float x) { return std::sin(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32)); }
inline float sin_cos_01(float x) { return std::sin(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32)); }
inline float cos_sin_01(float x) { return std::cos(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32)); }
inline float cos_cos_01(float x) { return std::cos(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32)); }
inline float sin_sin_sin_01(float x) { return std::sin(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32))); }
inline float sin_sin_cos_01(float x) { return std::sin(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32))); }
inline float sin_cos_sin_01(float x) { return std::sin(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32))); }
inline float sin_cos_cos_01(float x) { return std::sin(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32))); }
inline float cos_sin_sin_01(float x) { return std::cos(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32))); }
inline float cos_sin_cos_01(float x) { return std::cos(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32))); }
inline float cos_cos_sin_01(float x) { return std::cos(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32 + std::sin(x * 2 * plugin_base::pi32))); }
inline float cos_cos_cos_01(float x) { return std::cos(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32 + std::cos(x * 2 * plugin_base::pi32))); }

}