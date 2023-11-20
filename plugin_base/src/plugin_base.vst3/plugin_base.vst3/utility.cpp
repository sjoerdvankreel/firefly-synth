#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base.vst3/utility.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

FUID 
fuid_from_text(char const* text)
{
  FUID result;
  INF_ASSERT_EXEC(result.fromString(text));
  return result;
}

bool 
load_plugin_state(IBStream* stream, plugin_state& state)
{
  char byte;
  int read = 1;
  std::vector<char> data;
  while (stream->read(&byte, 1, &read) == kResultTrue && read == 1)
    data.push_back(byte);
  return plugin_io_load_state(data, state).ok();
}

}