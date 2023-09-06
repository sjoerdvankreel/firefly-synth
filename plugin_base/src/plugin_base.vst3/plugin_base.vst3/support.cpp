#include <plugin_base/utility.hpp>
#include <plugin_base.vst3/support.hpp>

using namespace Steinberg;

namespace infernal::base::vst3 {

FUID 
fuid_from_text(char const* text)
{
  FUID result;
  INF_ASSERT_EXEC(result.fromString(text));
  return result;
}

}