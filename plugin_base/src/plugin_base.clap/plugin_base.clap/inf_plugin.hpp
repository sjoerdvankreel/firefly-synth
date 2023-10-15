#pragma once

#include <plugin_base/gui/gui.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/block/host.hpp>
#include <plugin_base/shared/state.hpp>

#include <clap/helpers/plugin.hh>
#include <readerwriterqueue.h>

#include <memory>
#include <cstdint>

namespace plugin_base::clap {

inline int constexpr default_q_size = 4096;
typedef std::unique_ptr<plugin_topo>(*topo_factory)();

enum class sync_event_type { end_edit, begin_edit, value_changing };

// linear and log map to normalized, step maps to plain
class clap_value {
  double _value;
public:
  clap_value(clap_value const&) = default;
  clap_value& operator=(clap_value const&) = default;
  inline double value() const { return _value; }
  explicit clap_value(double value) : _value(value) {}
};

// sync audio <-> ui
struct sync_event
{
  int index = {};
  sync_event_type type;
  plain_value plain = {};
};

class inf_plugin:
public ::clap::helpers::Plugin<
  ::clap::helpers::MisbehaviourHandler::Ignore,
  ::clap::helpers::CheckingLevel::Maximal>,
public gui_listener,
public any_state_listener,
public juce::Timer
{
  typedef moodycamel::ReaderWriterQueue<sync_event, default_q_size> event_queue;

  plugin_engine _engine;
  plugin_state _gui_state = {};
  std::unique_ptr<plugin_gui> _gui = {};
  std::vector<int> _block_automation_seen = {};
  std::unique_ptr<event_queue> _to_gui_events = {};
  std::unique_ptr<event_queue> _to_audio_events = {};

  // Pull in values from audio->main regardless of whether ui is present.
  void timerCallback() override;
  void push_to_gui(int index, clap_value clap);
  void push_to_audio(int index, plain_value plain);
  void push_to_audio(int index, sync_event_type type);
  void process_gui_to_audio_events(clap_output_events_t const* out);

public:
  ~inf_plugin();
  INF_PREVENT_ACCIDENTAL_COPY(inf_plugin);
  inf_plugin(
    clap_plugin_descriptor const* clap_desc, 
    clap_host const* host, plugin_desc const* desc);
  
  bool implementsGui() const noexcept override { return true; }
  bool implementsState() const noexcept override { return true; }
  bool implementsParams() const noexcept override { return true; }
  bool implementsNotePorts() const noexcept override { return true; }
  bool implementsAudioPorts() const noexcept override { return true; }
  bool implementsThreadPool() const noexcept override { return true; }

  bool stateSave(clap_ostream const* stream) noexcept override;
  bool stateLoad(clap_istream const* stream) noexcept override;
  
#if (defined __linux__) || (defined  __FreeBSD__)
  void onPosixFd(int fd, int flags) noexcept override;
  bool implementsPosixFdSupport() const noexcept override { return true; }
#endif

  void threadPoolExec(uint32_t task_index) noexcept override 
  { _engine.process_voice(task_index, true); }  
  bool thread_pool_voice_processor(plugin_engine& engine)
  { return _host.threadPoolRequestExec(engine.state().desc().plugin->polyphony); }

  bool guiShow() noexcept override;
  bool guiHide() noexcept override;
  void guiDestroy() noexcept override;
  bool guiSetScale(double scale) noexcept override;
  bool guiSetParent(clap_window const* window) noexcept override;
  bool guiCreate(char const* api, bool is_floating) noexcept override;
  bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
  bool guiGetSize(uint32_t* width, uint32_t* height) noexcept override;
  bool guiAdjustSize(uint32_t* width, uint32_t* height) noexcept override;
  bool guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept override;
  bool guiIsApiSupported(char const* api, bool is_floating) noexcept override;
  bool guiCanResize() const noexcept override { return true; }

  std::int32_t getParamIndexForParamId(clap_id param_id) const noexcept override;
  bool getParamInfoForParamId(clap_id param_id, clap_param_info* info) const noexcept override;

  bool paramsValue(clap_id param_id, double* value) noexcept override;
  bool paramsInfo(std::uint32_t index, clap_param_info* info) const noexcept override;
  bool paramsTextToValue(clap_id param_id, char const* display, double* value) noexcept override;
  void paramsFlush(clap_input_events const* in, clap_output_events const* out) noexcept override;
  bool paramsValueToText(clap_id param_id, double value, char* display, std::uint32_t size) noexcept override;
  std::uint32_t paramsCount() const noexcept override { return _engine.state().desc().param_count; }

  std::uint32_t notePortsCount(bool is_input) const noexcept override { return is_input? 1: 0;}
  bool notePortsInfo(std::uint32_t index, bool is_input, clap_note_port_info* info) const noexcept override;
  std::uint32_t audioPortsCount(bool is_input) const noexcept override;
  bool audioPortsInfo(std::uint32_t index, bool is_input, clap_audio_port_info* info) const noexcept override;

  bool init() noexcept override;
  void deactivate() noexcept override { _engine.deactivate();  }
  clap_process_status process(clap_process const* process) noexcept override;
  bool activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept override;

  void gui_changing(int index, plain_value plain) override;
  void any_state_changed(int index, plain_value plain) override;
  void gui_end_changes(int index) override { push_to_audio(index, sync_event_type::end_edit); }
  void gui_begin_changes(int index) override { push_to_audio(index, sync_event_type::begin_edit); }
};

}
