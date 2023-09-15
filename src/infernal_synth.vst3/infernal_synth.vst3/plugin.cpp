#include <infernal_synth/synth.hpp>
#include <infernal_synth/plugin.hpp>

#include <plugin_base.vst3/support.hpp>
#include <plugin_base.vst3/inf_component.hpp>
#include <plugin_base.vst3/inf_controller.hpp>

#include <public.sdk/source/main/pluginfactory.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace infernal_synth;
using namespace plugin_base::vst3;

#if INF_IS_FX
#define PLUG_TYPE PlugType::kFx
#define INF_SYNTH_CONTROLLER_ID "891815F992F548248536BDFFF3EE8938"
#else
#define PLUG_TYPE PlugType::kInstrument
#define INF_SYNTH_CONTROLLER_ID "E5EC671A225942D5B03FE8131DB8CD46"
#endif

static FUnknown*
controller_factory(void*)
{
  auto result = new inf_controller(synth_topo());
  return static_cast<IEditController*>(result);
}

static FUnknown*
component_factory(void*)
{ 
  FUID controller_id(fuid_from_text(INF_SYNTH_CONTROLLER_ID));
  auto result = new inf_component(synth_topo(), controller_id);
  return static_cast<IAudioProcessor*>(result);
}

BEGIN_FACTORY_DEF(INF_SYNTH_VENDOR_NAME, INF_SYNTH_VENDOR_URL, INF_SYNTH_VENDOR_MAIL)
  DEF_CLASS2(
    INLINE_UID_FROM_FUID(fuid_from_text(INF_SYNTH_ID)), PClassInfo::kManyInstances,
    kVstAudioEffectClass, INF_SYNTH_FULL_NAME, Vst::kDistributable, 
    PLUG_TYPE, INF_SYNTH_VERSION_TEXT, kVstVersionString, component_factory);
  DEF_CLASS2(
    INLINE_UID_FROM_FUID(fuid_from_text(INF_SYNTH_CONTROLLER_ID)), PClassInfo::kManyInstances,
    kVstComponentControllerClass, INF_SYNTH_FULL_NAME, 0, "", INF_SYNTH_VERSION_TEXT,
    kVstVersionString, controller_factory)
END_FACTORY
