#pragma once

#include <cmath>

// https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
namespace firefly_synth
{

// targeted to stereo use
class state_var_filter
{
  double _k;
  double _ic1eq[2];
  double _ic2eq[2];
  double _a1, _a2, _a3;
  double _m0, _m1, _m2;
  void init(double w, double res, double g, double a);

public:
  void clear();
  double next(int ch, double in);

  void init_lpf(double w, double res);
  void init_hpf(double w, double res);
  void init_bpf(double w, double res);
  void init_bsf(double w, double res);
  void init_apf(double w, double res);
  void init_peq(double w, double res);
  void init_bll(double w, double res, double db_gain);
  void init_lsh(double w, double res, double db_gain);
  void init_hsh(double w, double res, double db_gain);
};

inline void 
state_var_filter::clear()
{
  _ic1eq[0] = 0;
  _ic1eq[1] = 0;
  _ic2eq[0] = 0;
  _ic2eq[1] = 0;

  _k = 0;
  _a1 = _a2 = _a3 = 0;
  _m0 = _m1 = _m2 = 0;
}

inline void
state_var_filter::init(double w, double res, double g, double a)
{
  _k = (2 - 2 * res) / a;
  _a1 = 1 / (1 + g * (g + _k));
  _a2 = g * _a1;
  _a3 = g * _a2;
}

inline void
state_var_filter::init_lpf(double w, double res)
{
  init(w, res, std::tan(w), 1);
  _m0 = 0; _m1 = 0; _m2 = 1;
}

inline void
state_var_filter::init_hpf(double w, double res)
{
  init(w, res, std::tan(w), 1);
  _m0 = 1; _m1 = -_k; _m2 = -1;
}

inline void
state_var_filter::init_bpf(double w, double res)
{
  init(w, res, std::tan(w), 1);
  _m0 = 0; _m1 = 1; _m2 = 0;
}

inline void
state_var_filter::init_bsf(double w, double res)
{
  init(w, res, std::tan(w), 1);
  _m0 = 1; _m1 = -_k; _m2 = 0;
}

inline void
state_var_filter::init_apf(double w, double res)
{
  init(w, res, std::tan(w), 1);
  _m0 = 1; _m1 = -2 * _k; _m2 = 0;
}

inline void
state_var_filter::init_peq(double w, double res)
{
  init(w, res, std::tan(w), 1);
  _m0 = 1; _m1 = -_k; _m2 = -2;
}

inline void
state_var_filter::init_bll(double w, double res, double db_gain)
{
  double a = std::pow(10.0, db_gain / 40.0);
  init(w, res, std::tan(w), a);
  _m0 = 1; _m1 = _k * (a * a - 1); _m2 = 0;
}

inline void
state_var_filter::init_lsh(double w, double res, double db_gain)
{
  double a = std::pow(10.0, db_gain / 40.0);
  init(w, res, std::tan(w) / std::sqrt(a), 1);
  _m0 = 1; _m1 = _k * (a - 1); _m2 = a * a - 1;
}

inline void
state_var_filter::init_hsh(double w, double res, double db_gain)
{
  double a = std::pow(10.0, db_gain / 40.0);
  init(w, res, std::tan(w) * std::sqrt(a), 1);
  _m0 = a * a; _m1 = _k * (1 - a) * a; _m2 = 1 - a * a;
}

inline double
state_var_filter::next(int ch, double in)
{
  double v0 = in;
  double v3 = v0 - _ic2eq[ch];
  double v1 = _a1 * _ic1eq[ch] + _a2 * v3;
  double v2 = _ic2eq[ch] + _a2 * _ic1eq[ch] + _a3 * v3;
  _ic1eq[ch] = 2 * v1 - _ic1eq[ch];
  _ic2eq[ch] = 2 * v2 - _ic2eq[ch];
  return _m0 * v0 + _m1 * v1 + _m2 * v2;
}

}