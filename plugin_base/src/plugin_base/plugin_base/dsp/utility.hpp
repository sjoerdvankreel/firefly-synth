#pragma once

#include <plugin_base/topo/domain.hpp>
#include <plugin_base/shared/jarray.hpp>

#include <cmath>
#include <utility>
#include <cassert>
#include <cstdint>

namespace plugin_base {

int const midi_middle_c = 60;

std::pair<std::uint32_t, std::uint32_t> disable_denormals();
void restore_denormals(std::pair<std::uint32_t, std::uint32_t> state);

// for smoothing midi or host granularity changes
class block_filter
{
  int _length;
  int _pos = 0;
  float _to = 0;
  float _from = 0;
  float _current = 0;
public:
  void set(float val);
  std::pair<float, bool> next();
  void init(float rate, float duration);
  
  block_filter(): _length(0) {}
  float current() const { return _current; }
  void current(float current) { _current = current; }
  bool active() const { return _pos < _length; }
  block_filter(float rate, float duration, float default_):
  _length(duration * rate), _from(default_), _to(default_), _current(default_) {}
};

inline void
block_filter::set(float val)
{
  _pos = 0;
  _to = val;
  _from = _current;
}

inline void 
block_filter::init(float rate, float duration)
{
  int new_length = duration * rate;
  if(new_length == _length) return;
  _pos = 0;
  _length = new_length;
}

inline std::pair<float, bool>
block_filter::next()
{
  if(_pos >= _length) return std::make_pair(_to, false);
  _current = _from + (_to - _from) * (_pos++ / (float)_length);
  return std::make_pair(_current, true);
}

// for smoothing internal control signals
// https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html
class cv_filter
{
  float _a = 0;
  float _b = 0;
  float _z = 0;
  float _sample_rate = 0;
  float _response_time = 0;
  std::int64_t _active_samples = 0;
  std::int64_t _response_samples = 0;
public:
  float next(float in);
  void init(float sample_rate, float response_time);

  float current() const { return _z; }
  void current(float val) { _z = val; _active_samples = 0; }
  bool active() const { return _active_samples < _response_samples; }
};

inline float
cv_filter::next(float in)
{
  _z = (in * _b) + (_z * _a);
  if(in != _z) _active_samples = 0;
  _active_samples++;
  return _z;
}

inline void
cv_filter::init(float sample_rate, float response_time)
{
  // no need to throw out the history if we're not updating
  if(_sample_rate == sample_rate && _response_time == response_time) 
    return;

  _active_samples = 0;
  _sample_rate = sample_rate;
  _response_time = response_time;
  _response_samples = (std::int64_t)std::ceil(response_time * sample_rate);
  _a = std::exp(-2.0f * pi32 / (response_time * sample_rate));
  _b = 1.0f - _a;
  _z = 0.0f;
}

// https://www.dsprelated.com/freebooks/filters/DC_Blocker.html
class dc_filter
{
  double _x0[2];
  double _y0[2];
  double _r = 0;
public:
  float next(int ch, float in);
  void init(float sr, float freq_hz);
};

inline void 
dc_filter::init(float sr, float freq_hz)
{
  _x0[0] = 0;
  _x0[1] = 0;
  _y0[0] = 0;
  _y0[1] = 0;
  _r = 1 - (pi32 * 2 * freq_hz / sr);
}

inline float 
dc_filter::next(int ch, float in)
{
  float out = in - _x0[ch] + _y0[ch] * _r;
  _x0[ch] = in;
  _y0[ch] = out;
  return out;
}

inline std::uint32_t
fast_rand_seed(int seed)
{ return std::numeric_limits<uint32_t>::max() / seed; }

// https://en.wikipedia.org/wiki/Lehmer_random_number_generator
inline float
fast_rand_next(std::uint32_t& state)
{
  float const max_int = static_cast<float>(std::numeric_limits<std::int32_t>::max());
  state = static_cast<std::uint64_t>(state) * 48271 % 0x7fffffff;
  return static_cast<float>(state) / max_int;
}

inline double 
check_unipolar(double val)
{
  // make it debug breakable
#ifdef NDEBUG
  return val;
#else
  if (0 <= val && val <= 1) return val;
  assert(0 <= val && val <= 1);
  return val;
#endif
}

inline double
check_bipolar(double val)
{
  // make it debug breakable
#ifdef NDEBUG
  return val;
#else
  if (-1 <= val && val <= 1) return val;
  assert(-1 <= val && val <= 1);
  return val;
#endif;
}

inline float
check_unipolar(float val)
{
  // make it debug breakable
#ifdef NDEBUG
  return val;
#else
  if (-1e-5 <= val && val <= 1 + 1e-5) return val;
  assert(-1e-5 <= val && val <= 1 + 1e-5);
  return val;
#endif
}

inline float
check_bipolar(float val)
{
  // make it debug breakable
#ifdef NDEBUG
  return val;
#else
  if (-1 - 1e-5 <= val && val <= 1 + 1e-5) return val;
  assert(-1 - 1e-5 <= val && val <= 1 + 1e-5);
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

inline float unipolar_to_bipolar(float v) { return check_unipolar(v) * 2 - 1; }
inline float bipolar_to_unipolar(float v) { return (check_bipolar(v) + 1) * 0.5f; }

inline float 
phase_increment(float freq, float rate) 
{ return freq / rate; }
inline bool
increment_and_wrap_phase(float& phase, float inc)
{ phase += inc; bool wrapped = phase >= 1.0f; phase -= std::floor(phase); return wrapped; }
inline bool
increment_and_wrap_phase(float& phase, float freq, float rate)
{ return increment_and_wrap_phase(phase, phase_increment(freq, rate)); }

template <int Channel> inline float
mono_pan_sqrt(float panning)
{
  static_assert(Channel == 0 || Channel == 1);
  if constexpr (Channel == 1) return std::sqrt(panning);
  else return std::sqrt(1.0f - panning);
}

inline float
stereo_balance2(int channel, float balance)
{
  assert(channel == 0 || channel == 1);
  assert(-1 <= balance && balance <= 1);
  if (channel == 0 && balance <= 0) return 1.0f;
  if (channel == 1 && balance >= 0) return 1.0f;
  if (channel == 0) return 1.0f - balance;
  return 1.0f + balance;
}

template <int Channel> inline float
stereo_balance(float balance)
{
  static_assert(Channel == 0 || Channel == 1);
  assert(-1 <= balance && balance <= 1);
  if constexpr (Channel == 0)
    return 1.0f - std::clamp(balance, 0.0f, 1.0f);
  else 
    return 1.0f + std::clamp(balance, -1.0f, 0.0f);
}

}