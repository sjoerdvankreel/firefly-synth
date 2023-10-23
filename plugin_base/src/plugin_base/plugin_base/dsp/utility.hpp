#pragma once

#include <plugin_base/shared/jarray.hpp>

#include <cmath>
#include <cassert>
#include <cstdint>

namespace plugin_base {

inline float constexpr pi32 = 3.14159265358979323846264338327950288f;
inline double constexpr pi64 = 3.14159265358979323846264338327950288;

template <class T>
inline void check_unipolar(T val)
{ assert((T)0 <= val && val <= (T)1); }

template <class T>
inline void check_bipolar(T val)
{ assert((T)-1 <= val && val <= (T)1); }

inline float
timesig_to_freq(float bpm, float num, float den)
{ return bpm / (60.0f * 4.0f * num / den); }
inline float
timesig_to_time(float bpm, float num, float den)
{ return 1.0f / timesig_to_freq(bpm, num, den); }

std::pair<std::uint32_t, std::uint32_t> disable_denormals();
void restore_denormals(std::pair<std::uint32_t, std::uint32_t> state);

inline float
balance(int channel, float value)
{
  assert(-1 <= value && value <= 1);
  assert(channel == 0 || channel == 1);
  float pan = (value + 1) * 0.5f;
  return channel == 0 ? 1.0f - pan: pan;
}

inline float
note_to_freq(int oct, int note, float cent, int key)
{
  int const middle_c = 60;
  assert(0 <= oct && oct <= 9);
  assert(0 <= note && note <= 11);
  assert(-1 <= cent && cent <= 1);
  float pitch = (12 * oct + note) + cent + (key - middle_c);
  return 440.0f * std::pow(2.0f, (pitch - 69.0f) / 12.0f);
}

}