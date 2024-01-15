#include <plugin_base/shared/graph_data.hpp>

namespace plugin_base {

void
graph_data::init(graph_data const& rhs)
{
  _type = rhs.type();
  _stroke = rhs.stroke();
  _bipolar = rhs.bipolar();
  _partitions = rhs.partitions();
  switch (rhs.type())
  {
  case graph_data_type::na: break;
  case graph_data_type::off: break;
  case graph_data_type::scalar: _scalar = rhs.scalar(); break;
  case graph_data_type::audio: _audio = jarray<float, 2>(rhs.audio()); break;
  case graph_data_type::series: _series = jarray<float, 1>(rhs.series()); break;
  case graph_data_type::multi_stereo: _multi_stereo = rhs.multi_stereo(); break;
  default: assert(false);
  }
}

}