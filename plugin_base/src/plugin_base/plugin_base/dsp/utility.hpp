#pragma once

#include <plugin_base/topo/domain.hpp>
#include <plugin_base/shared/jarray.hpp>

#include <cmath>
#include <utility>
#include <cassert>
#include <cstdint>

namespace plugin_base {

int const midi_middle_c = 60;

// for smoothing midi changes
class midi_filter
{
  int _length;
  int _pos = 0;
  float _to = 0;
  float _from = 0;
  float _current = 0;
public:
  void set(float val);
  std::pair<float, bool> next();
  midi_filter(float rate, float freq, float default_):
  _length(rate / freq), _to(default_) {}
};

inline void
midi_filter::set(float val)
{
  _pos = 0;
  _to = val;
  _from = _current;
}

inline std::pair<float, bool>
midi_filter::next()
{
  if(_pos == _length) return std::make_pair(_to, false);
  _current = _from + (_to - _from) * (_pos++ / (float)_length);
  return std::make_pair(_current, true);
}

// for smoothing control signals
// https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html
class cv_filter
{
  float a = 0;
  float b = 0;
  float z = 0;
public:
  float next(float in);
  void set(float sample_rate, float response_time);
};

inline float
cv_filter::next(float in)
{
  z = (in * b) + (z * a);
  return z;
}

inline void
cv_filter::set(float sample_rate, float response_time)
{
  a = std::exp(-2.0f * pi32 / (response_time * sample_rate));
  b = 1.0f - a;
  z = 0.0f;
}

std::pair<std::uint32_t, std::uint32_t> disable_denormals();
void restore_denormals(std::pair<std::uint32_t, std::uint32_t> state);

template <class T> 
inline T check_unipolar(T val)
{
  // make it debug breakable
#ifdef NDEBUG
  return val;
#else
  if ((T)0 <= val && val <= (T)1) return val;
  assert((T)0 <= val && val <= (T)1);
  return val;
#endif
}

template <class T> 
inline T check_bipolar(T val)
{
  // make it debug breakable
#ifdef NDEBUG
  return val;
#else
  if ((T)-1 <= val && val <= (T)1) return val;
  assert((T)-1 <= val && val <= (T)1);
  return val;
#endif;
}

inline float mix_signal(float mix, float dry, float wet) 
{ return (1.0f - mix) * dry + mix * wet; }
inline float pitch_to_freq(float pitch)
{ return 440.0f * std::pow(2.0f, (pitch - 69.0f) / 12.0f); }

inline float timesig_to_freq(float bpm, timesig const& sig) 
{ return bpm / (60.0f * 4.0f * sig.num / sig.den); }
inline float timesig_to_time(float bpm, timesig const& sig) 
{ return 1.0f / timesig_to_freq(bpm, sig); }

inline float unipolar_to_bipolar(float v) { return v * 2 - 1; }
inline float bipolar_to_unipolar(float v) { return (v + 1) * 0.5f; }

inline float 
phase_increment(float freq, float rate) 
{ return freq / rate; }
inline bool
increment_and_wrap_phase(float& phase, float inc)
{ phase += inc; bool wrapped = phase >= 1.0f; phase -= std::floor(phase); return wrapped; }
inline bool
increment_and_wrap_phase(float& phase, float freq, float rate)
{ return increment_and_wrap_phase(phase, phase_increment(freq, rate)); }

inline float
mono_pan_sqrt3(int channel, float panning)
{
  assert(channel == 0 || channel == 1);
  assert(0 <= panning && panning <= 1);
  if (channel == 1) return std::sqrt(panning);
  return std::sqrt(1 - panning);
}

inline float
stereo_balance(int channel, float balance)
{
  assert(channel == 0 || channel == 1);
  assert(-1 <= balance && balance <= 1);
  if(channel == 0 && balance <= 0) return 1.0f;
  if(channel == 1 && balance >= 0) return 1.0f;
  if(channel == 0) return 1.0f - balance;
  return 1.0f + balance;
}

}