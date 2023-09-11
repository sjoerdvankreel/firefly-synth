#pragma once
#include <plugin_base/gui.hpp>
#include <plugin_base/engine.hpp>
#include <clap/helpers/plugin.hh>
#include <readerwriterqueue.h>
#include <memory>
#include <cstdint>

namespace plugin_base::clap {

inline int constexpr default_queue_size = 4096;

enum class param_queue_event_type { end_edit, begin_edit, value_changing };
struct param_queue_event
{
  int param_index = {};
  param_value value = {};
  param_queue_event_type type = {};
};

class plugin:
public ::clap::helpers::Plugin<
  // I want to use terminate but Maximal fires host-checks on reaper w.r.t. resizing. Not workable.
  ::clap::helpers::MisbehaviourHandler::Ignore,
  ::clap::helpers::CheckingLevel::Maximal>,
public any_param_ui_listener,
public juce::Timer
{
  plugin_engine _engine;
  std::unique_ptr<plugin_gui> _gui = {};
  plugin_topo_factory const _topo_factory = {};
  jarray3d<param_value> _ui_state = {}; // Copy of engine state on the ui thread.
  std::vector<int> _block_automation_seen = {}; // Only push the first event in per-block automation.
  // These have an initial capacity but *will* allocate if it is exceeded because we push() not try_push().
  // By pointer rather than value to prevent some compiler warnings regarding padding.
  std::unique_ptr<moodycamel::ReaderWriterQueue<param_queue_event, default_queue_size>> _to_ui_events;
  std::unique_ptr<moodycamel::ReaderWriterQueue<param_queue_event, default_queue_size>> _to_audio_events;

  // Syncing audio <-> main.
  // We pull in values from audio->main regardless of whether ui is present/visible.
  void timerCallback() override;
  void push_to_ui(int param_index, double clap_value);
  void push_to_audio(int param_index, param_value base_value);
  void push_to_audio(int param_index, param_queue_event_type type);
  void process_ui_to_audio_events(const clap_output_events_t* out);

public:
  ~plugin() { stopTimer(); }
  plugin(clap_plugin_descriptor const* desc, clap_host const* host, plugin_topo_factory factory);
  
  bool implementsGui() const noexcept override { return true; }
  bool implementsParams() const noexcept override { return true; }
  bool implementsNotePorts() const noexcept override { return true; }
  bool implementsAudioPorts() const noexcept override { return true; }

  std::int32_t getParamIndexForParamId(clap_id param_id) const noexcept override;
  bool getParamInfoForParamId(clap_id param_id, clap_param_info* info) const noexcept override;

  bool guiShow() noexcept override;
  bool guiHide() noexcept override;
  void guiDestroy() noexcept override;
  bool guiSetParent(clap_window const* window) noexcept override;
  bool guiCreate(char const* api, bool is_floating) noexcept override;
  bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
  bool guiGetSize(uint32_t* width, uint32_t* height) noexcept override;
  bool guiAdjustSize(uint32_t* width, uint32_t* height) noexcept override;
  bool guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept override;
  bool guiCanResize() const noexcept override { return true; }
  bool guiIsApiSupported(char const* api, bool is_floating) noexcept override { return !is_floating; }

  bool paramsValue(clap_id param_id, double* value) noexcept override;
  bool paramsInfo(std::uint32_t param_index, clap_param_info* info) const noexcept override;
  bool paramsTextToValue(clap_id param_id, char const* display, double* value) noexcept override;
  void paramsFlush(clap_input_events const* in, clap_output_events const* out) noexcept override;
  bool paramsValueToText(clap_id param_id, double value, char* display, std::uint32_t size) noexcept override;
  std::uint32_t paramsCount() const noexcept override { return _engine.desc().param_mappings.size(); }

  std::uint32_t notePortsCount(bool is_input) const noexcept override { return is_input? 1: 0;}
  bool notePortsInfo(std::uint32_t index, bool is_input, clap_note_port_info* info) const noexcept override;
  std::uint32_t audioPortsCount(bool is_input) const noexcept override;
  bool audioPortsInfo(std::uint32_t index, bool is_input, clap_audio_port_info* info) const noexcept override;

  bool init() noexcept override; // Need to start timer on the main thread. Ctor is not guaranteed to run there.
  void deactivate() noexcept override { _engine.deactivate();  }
  clap_process_status process(clap_process const* process) noexcept override;
  bool activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept override;

  void ui_param_changing(int param_index, param_value value) override;
  void ui_param_end_changes(int param_index) override { push_to_audio(param_index, param_queue_event_type::end_edit); }
  void ui_param_begin_changes(int param_index) override { push_to_audio(param_index, param_queue_event_type::begin_edit); }
};

}
