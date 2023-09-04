#include <infernal.synth/plugin.hpp>
#include <infernal.base.vst3/support.hpp>
#include <public.sdk/source/main/pluginfactory.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace infernal::base::vst3;

#if INF_SYNTH_FX
#define PLUG_TYPE PlugType::kFx
#define INF_SYNTH_CONTROLLER_ID "891815F992F548248536BDFFF3EE8938"
#else
#define PLUG_TYPE PlugType::kInstrument
#define INF_SYNTH_CONTROLLER_ID "E5EC671A225942D5B03FE8131DB8CD46"
#endif

BEGIN_FACTORY_DEF(INF_SYNTH_VENDOR_NAME, INF_SYNTH_VENDOR_URL, INF_SYNTH_VENDOR_MAIL)
  DEF_CLASS2(
    INLINE_UID_FROM_FUID(fuid_from_text(INF_SYNTH_ID)), PClassInfo::kManyInstances,
    kVstAudioEffectClass, INF_SYNTH_FULL_NAME, Vst::kDistributable, 
    PLUG_TYPE, INF_SYNTH_VERSION_TEXT, kVstVersionString, nullptr);
  DEF_CLASS2(
    INLINE_UID_FROM_FUID(fuid_from_text(INF_SYNTH_CONTROLLER_ID)), PClassInfo::kManyInstances,
    kVstComponentControllerClass, INF_SYNTH_FULL_NAME, 0, "", INF_SYNTH_VERSION_TEXT,
    kVstVersionString, nullptr)
END_FACTORY
