#pragma once

#include <plugin_base/dsp/utility.hpp>
#include <cmath>

// trigonometric stuff
// x01 doesnt have to be in [0, 1], but it will be scaled by 2pi
namespace firefly_synth
{

inline float ff_sin(float x01) { return std::sin(x01 * 2 * plugin_base::pi32); }
inline float ff_cos(float x01) { return std::cos(x01 * 2 * plugin_base::pi32); }
inline float ff_sin_sin(float x01) { return std::sin(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32)); }
inline float ff_sin_cos(float x01) { return std::sin(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32)); }
inline float ff_cos_sin(float x01) { return std::cos(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32)); }
inline float ff_cos_cos(float x01) { return std::cos(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32)); }
inline float ff_sin_sin_sin(float x01) { return std::sin(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32))); }
inline float ff_sin_sin_cos(float x01) { return std::sin(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32))); }
inline float ff_sin_cos_sin(float x01) { return std::sin(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32))); }
inline float ff_sin_cos_cos(float x01) { return std::sin(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32))); }
inline float ff_cos_sin_sin(float x01) { return std::cos(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32))); }
inline float ff_cos_sin_cos(float x01) { return std::cos(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32))); }
inline float ff_cos_cos_sin(float x01) { return std::cos(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32 + std::sin(x01 * 2 * plugin_base::pi32))); }
inline float ff_cos_cos_cos(float x01) { return std::cos(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32 + std::cos(x01 * 2 * plugin_base::pi32))); }

}