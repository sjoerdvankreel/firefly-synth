#include <plugin_base.vst3/utility.hpp>
#include <plugin_base/shared/utility.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

FUID 
fuid_from_text(char const* text)
{
  FUID result;
  PB_ASSERT_EXEC(result.fromString(text));
  return result;
}

std::vector<char>
load_ibstream(IBStream* stream)
{
  char byte;
  int read = 1;
  std::vector<char> data = {};
  while (stream->read(&byte, 1, &read) == kResultTrue && read == 1)
    data.push_back(byte);
  return data;
}

}