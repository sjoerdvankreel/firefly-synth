#pragma once

#include <pluginterfaces/base/funknown.h>

namespace plugin_base::vst3 {

Steinberg::FUID 
fuid_from_text(char const* text);

}