#include <plugin_base.vst3/inf_param.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace plugin_base::vst3 {

void 
inf_param::toString(ParamValue normalized, String128 string) const
{
  auto text = _topo->domain.normalized_to_text(normalized_value(normalized));
  from_8bit_string(string, sizeof(String128) / sizeof(string[0]), text.c_str());
}

bool 
inf_param::fromString(TChar const* string, ParamValue& normalized) const
{
  normalized_value base_normalized;
  std::string text(to_8bit_string(string));
  if(!_topo->domain.text_to_normalized(text, base_normalized)) return false;
  normalized = base_normalized.value();
  return true;
}

}