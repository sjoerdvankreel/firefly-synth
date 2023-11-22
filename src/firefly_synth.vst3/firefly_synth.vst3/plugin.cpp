#include <firefly_synth/synth.hpp>
#include <firefly_synth/plugin.hpp>

#include <plugin_base/gui/utility.hpp>
#include <plugin_base.vst3/utility.hpp>
#include <plugin_base.vst3/pb_component.hpp>
#include <plugin_base.vst3/pb_controller.hpp>

#include <public.sdk/source/main/pluginfactory.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace plugin_base;
using namespace firefly_synth;
using namespace plugin_base::vst3;

#if INF_IS_FX
#define PLUG_TYPE PlugType::kFx
#define INF_SYNTH_CONTROLLER_ID "891815F992F548248536BDFFF3EE8938"
#else
#define PLUG_TYPE PlugType::kInstrument
#define INF_SYNTH_CONTROLLER_ID "E5EC671A225942D5B03FE8131DB8CD46"
#endif

static std::unique_ptr<plugin_topo> _topo = {};
static std::unique_ptr<plugin_desc> _desc = {};

bool
DeinitModule()
{
  juce::shutdownJuce_GUI();
  _desc.reset();
  _topo.reset();
  return true;
}

bool 
InitModule() 
{ 
  _topo = synth_topo();
  _desc = std::make_unique<plugin_desc>(_topo.get(), vst3_config::instance());
  juce::initialiseJuce_GUI();
  return true; 
}

static FUnknown*
controller_factory(void*)
{
  auto result = new pb_controller(_desc.get());
  return static_cast<IEditController*>(result);
}

static FUnknown*
component_factory(void*)
{
  FUID controller_id(fuid_from_text(INF_SYNTH_CONTROLLER_ID));
  auto result = new pb_component(_desc.get(), controller_id);
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
