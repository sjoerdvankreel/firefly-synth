#pragma once
#include <cmath>
#define INF_PI 3.14159265358979323846264338327950288

namespace infernal::base {

inline float
balance(int channel, float value)
{
  float pan = (value + 1) * 0.5f;
  return channel == 0 ? 1.0f - pan: pan;
}

inline float
note_to_frequency(int oct, int note, float cent)
{
  float pitch = 12 * oct + note + cent;
  return 440.0f * std::pow(2.0f, (pitch - 69.0f) / 12.0f);
}

}