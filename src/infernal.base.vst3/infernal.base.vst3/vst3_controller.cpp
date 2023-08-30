#include <infernal.base.vst3/vst3_controller.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

tresult PLUGIN_API 
vst3_controller::initialize(FUnknown* context)
{
  if(EditController::initialize(context) != kResultTrue) return kResultFalse;

  int unit_id = 1;
  for(int m = 0; m < _topo.modules.size(); m++)
    for (int i = 0; i < _topo.modules[m].count; i++)
    {
      UnitInfo info;
      info.id = unit_id++;
      info.parentUnitId = kRootUnitId;
      info.programListId = kNoProgramListId;
      info.name = 
    }

  return kResultTrue;
}

}