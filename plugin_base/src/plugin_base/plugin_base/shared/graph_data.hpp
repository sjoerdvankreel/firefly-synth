#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <cassert>

namespace plugin_base {

enum class graph_data_type { off, na, scalar, series, audio, multi_stereo };

class graph_data {
  bool _bipolar = false;
  graph_data_type _type = {};
  std::vector<std::string> _partitions = {};

  float _scalar = {};
  jarray<float, 2> _audio = {};
  jarray<float, 1> _series = {};
  std::vector<std::pair<float, float>> _multi_stereo = {};

  void init(graph_data const& rhs);

public:
  float scalar() const 
  { assert(_type == graph_data_type::scalar); return _scalar; }
  jarray<float, 2> const& audio() const 
  { assert(_type == graph_data_type::audio); return _audio; }
  jarray<float, 1> const& series() const 
  { assert(_type == graph_data_type::series); return _series; }
  std::vector<std::pair<float, float>> const& multi_stereo() const 
  { assert(_type == graph_data_type::multi_stereo); return _multi_stereo; }

  bool bipolar() const { return _bipolar; }
  graph_data_type type() const { return _type; }
  std::vector<std::string> const& partitions() const { return _partitions; }

  graph_data(graph_data const& rhs) { init(rhs); }
  graph_data& operator=(graph_data const& rhs) { init(rhs); return *this; }

  graph_data(graph_data_type type, std::vector<std::string> const& partitions):
  _partitions(partitions), _type(type) {}
  graph_data(jarray<float, 2> const& audio, std::vector<std::string> const& partitions) :
  _partitions(partitions), _bipolar(true), _type(graph_data_type::audio), _audio(audio) {}
  graph_data(float scalar, bool bipolar, std::vector<std::string> const& partitions):
  _partitions(partitions), _bipolar(bipolar), _type(graph_data_type::scalar), _scalar(scalar) {}
  graph_data(jarray<float, 1> const& series, bool bipolar, std::vector<std::string> const& partitions) :
  _partitions(partitions), _bipolar(bipolar), _type(graph_data_type::series), _series(series) {}
  graph_data(std::vector<std::pair<float, float>> const& multi_stereo, std::vector<std::string> const& partitions) :
    _partitions(partitions), _bipolar(false), _type(graph_data_type::multi_stereo), _multi_stereo(multi_stereo) {}
};

}
