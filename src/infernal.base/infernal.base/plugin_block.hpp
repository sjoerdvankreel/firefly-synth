#ifndef INFERNAL_BASE_PLUGIN_BLOCK_HPP
#define INFERNAL_BASE_PLUGIN_BLOCK_HPP

#include <vector>
#include <cstdint>

namespace infernal::base {

enum class note_event_type { on, off, cut };
struct param_value { union { float real; int step; }; };

struct note_id {
  int id;
  short key;
  short channel;
};

struct note_event {
  note_id id;
  float velocity;
  int frame_index;
  note_event_type type;
};

struct block_automation_event {
  param_value value;
  int runtime_param_index;
};

struct accurate_automation_event {
  float value;
  int frame_index;
  int runtime_param_index;
};

struct host_block_base {
  float bpm;
  int frame_count;
  std::int64_t stream_time;
  float* const* audio_output;
  float const* const* audio_input;
  std::vector<note_event> notes;
};

struct host_block: host_block_base {
  std::vector<block_automation_event> block_automation;
  std::vector<accurate_automation_event> accurate_automation;
};

struct plugin_block {
  int module_index;
  float*** module_cv;
  float**** module_audio;
  host_block_base const* host;
  float**** accurate_automation;
  param_value*** block_automation;
};

typedef void(*module_process)(plugin_block const& block);

}
#endif
