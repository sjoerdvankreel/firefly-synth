#include <plugin_base.vst3/pb_param.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace plugin_base::vst3 {

pb_param::
pb_param(plugin_state const* state, param_topo const* topo, int index, Steinberg::Vst::ParameterInfo const& info) :
Parameter(info), _index(index), _topo(topo), _state(state) {}

void 
pb_param::toString(ParamValue normalized, String128 string) const
{
  auto text = _state->normalized_to_text_at_index(false, _index, normalized_value(normalized));
  from_8bit_string(string, sizeof(String128) / sizeof(string[0]), text.c_str());
}

bool 
pb_param::fromString(TChar const* string, ParamValue& normalized) const
{
  normalized_value base_normalized;
  std::string text(to_8bit_string(string));
  if(!_state->text_to_normalized_at_index(false, _index, text, base_normalized)) return false;
  normalized = base_normalized.value();
  return true;
}

}