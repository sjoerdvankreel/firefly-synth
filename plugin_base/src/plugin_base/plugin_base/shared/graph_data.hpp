#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <cassert>

namespace plugin_base {

enum class graph_data_type { off, na, scalar, series, audio };

class graph_data {
  int _partitions = 1;
  bool _bipolar = false;
  graph_data_type _type = {};
  
  float _scalar = {};
  jarray<float, 2> _audio = {};
  jarray<float, 1> _series = {};

  void init(graph_data const& rhs);

public:
  bool bipolar() const { return _bipolar; }
  graph_data_type type() const { return _type; }
  int partitions() const { return _partitions; }

  float scalar() const 
  { assert(_type == graph_data_type::scalar); return _scalar; }
  jarray<float, 2> const& audio() const 
  { assert(_type == graph_data_type::audio); return _audio; }
  jarray<float, 1> const& series() const 
  { assert(_type == graph_data_type::series); return _series; }

  graph_data(graph_data const& rhs) { init(rhs); }
  graph_data& operator=(graph_data const& rhs) { init(rhs); return *this; }

  graph_data(graph_data_type type, int partitions = 1): 
  _partitions(partitions), _type(type) {}
  explicit graph_data(jarray<float, 2> const& audio, int partitions = 1) :
  _partitions(partitions), _bipolar(true), _type(graph_data_type::audio), _audio(audio) {}
  graph_data(float scalar, bool bipolar, int partitions = 1):
  _partitions(partitions), _bipolar(bipolar), _type(graph_data_type::scalar), _scalar(scalar) {}
  graph_data(jarray<float, 1> const& series, bool bipolar, int partitions = 1) :
  _partitions(partitions), _bipolar(bipolar), _type(graph_data_type::series), _series(series) {}
};

}
