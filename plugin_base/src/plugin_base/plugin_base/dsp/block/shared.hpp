#pragma once

#include <cstdint>

namespace plugin_base {

// note id or PCK (port is 0)
struct note_id final {
  int id;
  int key;
  int channel;
};

// shared host/plug
struct shared_block final {
  float bpm;
  float const* const* audio_in;
};

}