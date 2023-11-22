#pragma once

#include <plugin_base/shared/utility.hpp>

#if INF_IS_FX
#define INF_SYNTH_NAME "Infernal Synth FX"
#define INF_SYNTH_ID "740EA0FF92D44ABB947C6C94DEF45E2C"
#else
#define INF_SYNTH_NAME "Infernal Synth"
#define INF_SYNTH_ID "56AFFFF7565F48348C4359D5E6C856A4"
#endif

#define INF_SYNTH_VENDOR_NAME "Sjoerd van Kreel"
#define INF_SYNTH_VENDOR_MAIL "sjoerdvankreel@gmail.com"
#define INF_SYNTH_VENDOR_URL "https://sjoerdvankreel.github.io/infernal-synth"

#define INF_SYNTH_VERSION_MAJOR 2
#define INF_SYNTH_VERSION_MINOR 0
#define INF_SYNTH_REVERSE_URI "io.github.sjoerdvankreel.infernal_synth"
#define INF_SYNTH_VERSION_TEXT INF_VERSION_TEXT(INF_SYNTH_VERSION_MAJOR, INF_SYNTH_VERSION_MINOR)
#define INF_SYNTH_FULL_NAME INF_SYNTH_NAME " " INF_SYNTH_VERSION_TEXT
