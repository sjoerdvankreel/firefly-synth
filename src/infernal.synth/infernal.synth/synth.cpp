#include <infernal.synth/synth.hpp>

using namespace infernal::base;

namespace infernal::synth {

plugin_topo
synth_topo()
{
  plugin_topo result;
  result.polyphony = 32;
  result.type = plugin_type::synth;
  return result;
}

}
