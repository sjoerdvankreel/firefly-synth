#include <plugin_base/shared/graph_data.hpp>

namespace plugin_base {

void
graph_data::init(graph_data const& rhs)
{
  _type = rhs.type();
  _bipolar = rhs.bipolar();
  _partitions = rhs.partitions();
  switch (rhs.type())
  {
  case graph_data_type::na: break;
  case graph_data_type::off: break;
  case graph_data_type::scalar: _scalar = rhs.scalar(); break;
  case graph_data_type::audio: _audio = jarray<float, 2>(rhs.audio()); break;
  case graph_data_type::series: _series = jarray<float, 1>(rhs.series()); break;
  default: assert(false);
  }
}

}