#pragma once

#include <vector>
#include <cstdint>

namespace infernal::base {

enum class note_event_type { on, off, cut };
struct param_value final { union { float real; int step; }; };

struct note_id final {
  int id;
  short key;
  short channel;
};

struct note_event final {
  note_id id;
  float velocity;
  int frame_index;
  note_event_type type;
};

struct block_automation_event final {
  param_value value;
  int runtime_param_index;
};

struct accurate_automation_event final {
  float value;
  int frame_index;
  int runtime_param_index;
};

struct common_block final {
  float bpm;
  int frame_count;
  std::int64_t stream_time;
  float* const* audio_output;
  float const* const* audio_input;
  std::vector<note_event> notes;
};

struct host_block final {
  common_block common;
  std::vector<block_automation_event> block_automation;
  std::vector<accurate_automation_event> accurate_automation;
};

struct plugin_block final {
  int module_index;
  float sample_rate;
  float*** module_cv;
  float**** module_audio;
  common_block const* host;
  float**** accurate_automation;
  param_value*** block_automation;
};

typedef void(*module_process)(plugin_block const& block);

}
