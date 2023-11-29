#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <cassert>

namespace plugin_base {

enum class graph_data_type { empty, scalar, series, audio };

class graph_data {
  graph_data_type const _type;
  bool const _bipolar = false;
  
  float const _scalar = {};
  jarray<float, 2> const _audio = {};
  jarray<float, 1> const _series = {};

public:
  bool bipolar() const { return _bipolar; }
  graph_data_type type() const { return _type; }

  float scalar() const 
  { assert(_type == graph_data_type::scalar); return _scalar; }
  jarray<float, 2> const& audio() const 
  { assert(_type == graph_data_type::audio); return _audio; }
  jarray<float, 1> const& series() const 
  { assert(_type == graph_data_type::series); return _series; }

  graph_data(): _type(graph_data_type::empty) {}
  explicit graph_data(jarray<float, 2> const& audio) : 
  _type(graph_data_type::audio), _bipolar(true), _audio(audio) {}
  graph_data(float scalar, bool bipolar): 
  _type(graph_data_type::scalar), _bipolar(bipolar), _scalar(scalar) {}
  graph_data(jarray<float, 1> const& series, bool bipolar) : 
  _type(graph_data_type::series), _bipolar(bipolar), _series(series) {}
};

}
