#pragma once

#include <plugin_base/shared/utility.hpp>

#if INF_IS_FX
#define FF_SYNTH_NAME "Firefly Synth FX"
#define FF_SYNTH_ID "740EA0FF92D44ABB947C6C94DEF45E2C"
#else
#define FF_SYNTH_NAME "Firefly Synth"
#define FF_SYNTH_ID "56AFFFF7565F48348C4359D5E6C856A4"
#endif

#define FF_SYNTH_VENDOR_NAME "Sjoerd van Kreel"
#define FF_SYNTH_VENDOR_MAIL "sjoerdvankreel@gmail.com"
#define FF_SYNTH_VENDOR_URL "https://sjoerdvankreel.github.io/firefly-synth"

#define FF_SYNTH_VERSION_MAJOR 2
#define FF_SYNTH_VERSION_MINOR 0
#define FF_SYNTH_REVERSE_URI "io.github.sjoerdvankreel.firefly_synth"
#define FF_SYNTH_VERSION_TEXT FF_VERSION_TEXT(FF_SYNTH_VERSION_MAJOR, FF_SYNTH_VERSION_MINOR)
#define FF_SYNTH_FULL_NAME FF_SYNTH_NAME " " FF_SYNTH_VERSION_TEXT
