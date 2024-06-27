#include <firefly_synth/synth.hpp>
#include <firefly_synth/plugin.hpp>

#include <plugin_base/gui/utility.hpp>
#include <plugin_base.vst3/utility.hpp>
#include <plugin_base.vst3/pb_component.hpp>
#include <plugin_base.vst3/pb_controller.hpp>

#include <public.sdk/source/main/pluginfactory.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>

#ifndef PB_IS_FX
#error
#elif PB_IS_FX
#define FF_SYNTH_ID FF_SYNTH_FX_ID
#define FF_SYNTH_FULL_NAME FF_SYNTH_FX_FULL_NAME " VST3"
#define FF_SYNTH_PLUG_TYPE Steinberg::Vst::PlugType::kFx
#define FF_SYNTH_CONTROLLER_ID "D9E715AC2D964F0FB8E3B835BEE053E1"
#else
#define FF_SYNTH_PLUG_TYPE Steinberg::Vst::PlugType::kInstrument
#define FF_SYNTH_ID FF_SYNTH_INST_ID
#define FF_SYNTH_FULL_NAME FF_SYNTH_INST_FULL_NAME " VST3"
#define FF_SYNTH_CONTROLLER_ID "E5EC671A225942D5B03FE8131DB8CD46"
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace plugin_base;
using namespace firefly_synth;
using namespace plugin_base::vst3;

static std::unique_ptr<plugin_topo> _topo = {};

bool
DeinitModule()
{
  juce::shutdownJuce_GUI();
  _topo.reset();
  return true;
}

bool 
InitModule() 
{ 
  _topo = synth_topo(PB_IS_FX);
  juce::initialiseJuce_GUI();
  return true; 
}

static FUnknown*
controller_factory(void*)
{
  auto result = new pb_controller(_topo.get());
  return static_cast<IEditController*>(result);
}

static FUnknown*
component_factory(void*)
{
  FUID controller_id(fuid_from_text(FF_SYNTH_CONTROLLER_ID));
  auto result = new pb_component(_topo.get(), controller_id);
  return static_cast<IAudioProcessor*>(result);
}

// for param list generator
extern "C" PB_EXPORT plugin_topo const* 
pb_plugin_topo_create() { return synth_topo(PB_IS_FX).release(); }
extern "C" PB_EXPORT void
pb_plugin_topo_destroy(plugin_topo const* topo) { delete topo; }

BEGIN_FACTORY_DEF(FF_SYNTH_VENDOR_NAME, FF_SYNTH_VENDOR_URL, FF_SYNTH_VENDOR_MAIL)
  DEF_CLASS2(
    INLINE_UID_FROM_FUID(fuid_from_text(FF_SYNTH_ID)), PClassInfo::kManyInstances,
    kVstAudioEffectClass, FF_SYNTH_FULL_NAME, Vst::kDistributable, 
    FF_SYNTH_PLUG_TYPE, FF_SYNTH_VERSION_TEXT, kVstVersionString, component_factory);
  DEF_CLASS2(
    INLINE_UID_FROM_FUID(fuid_from_text(FF_SYNTH_CONTROLLER_ID)), PClassInfo::kManyInstances,
    kVstComponentControllerClass, FF_SYNTH_FULL_NAME, 0, "", FF_SYNTH_VERSION_TEXT,
    kVstVersionString, controller_factory)
END_FACTORY
