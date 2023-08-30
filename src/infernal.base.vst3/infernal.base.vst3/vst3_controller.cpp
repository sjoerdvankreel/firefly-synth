#include <infernal.base.vst3/vst3_controller.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

static void 
copy_to_vst_string(TChar* dest, int count, char const* source)
{
  memset(dest, 0, sizeof(*dest) * count);
  for(int i = 0; i < count && i < strlen(source); i++)
    dest[i] = source[i];
}

tresult PLUGIN_API 
vst3_controller::initialize(FUnknown* context)
{
  if(EditController::initialize(context) != kResultTrue) return kResultFalse;

  int unit_id = 1;
  for(int m = 0; m < _topo.runtime_modules.size(); m++)
  {
    UnitInfo info;
    info.id = unit_id++;
    info.parentUnitId = kRootUnitId;
    info.programListId = kNoProgramListId;
    memset(info.name, 0, sizeof(info.name));
    copy_to_vst_string(info.name, 127, _topo.runtime_modules[m].name.c_str());
  }

  return kResultTrue;
}

}