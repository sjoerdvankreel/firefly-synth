#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

#include <map>
#include <cassert>

namespace plugin_base {

enum class graph_data_type { empty, scalar, series, audio };

class graph_data {
  bool _bipolar = false;
  graph_data_type _type = {};
  
  float _scalar = {};
  jarray<float, 2> _audio = {};
  std::map<float, float> _series = {};

  void init(graph_data const& rhs);

public:
  bool bipolar() const { return _bipolar; }
  graph_data_type type() const { return _type; }

  float scalar() const 
  { assert(_type == graph_data_type::scalar); return _scalar; }
  jarray<float, 2> const& audio() const 
  { assert(_type == graph_data_type::audio); return _audio; }
  std::map<float, float> const& series() const 
  { assert(_type == graph_data_type::series); return _series; }

  graph_data(graph_data const& rhs) { init(rhs); }
  graph_data& operator=(graph_data const& rhs) { init(rhs); return *this; }

  graph_data(): _type(graph_data_type::empty) {}
  explicit graph_data(jarray<float, 2> const& audio) :
  _bipolar(true), _type(graph_data_type::audio), _audio(audio) {}
  graph_data(float scalar, bool bipolar): 
  _bipolar(bipolar), _type(graph_data_type::scalar), _scalar(scalar) {}
  graph_data(std::map<float, float> const& series, bool bipolar) :
  _bipolar(bipolar), _type(graph_data_type::series), _series(series) {}
  graph_data(jarray<float, 1> const& series, bool bipolar) : 
  _bipolar(bipolar), _type(graph_data_type::series), _series(linear_map_series_x(series.data())) {}
};

}
