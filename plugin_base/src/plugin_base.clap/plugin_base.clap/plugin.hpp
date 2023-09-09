#pragma once
#include <plugin_base/engine.hpp>
#include <clap/helpers/plugin.hh>
#include <cstdint>

namespace plugin_base::clap {

class plugin:
public ::clap::helpers::Plugin<
  ::clap::helpers::MisbehaviourHandler::Terminate, 
  ::clap::helpers::CheckingLevel::Maximal>
{
  plugin_engine _engine;
  jarray3d<param_value> _ui_state = {}; // Copy of engine state on the ui thread.
  std::vector<int> _block_automation_seen = {}; // Only push the first event in per-block automation.
public:
  plugin(clap_plugin_descriptor const* desc, clap_host const* host, plugin_topo_factory factory);
  
  bool implementsParams() const noexcept override { return true; }
  bool implementsNotePorts() const noexcept override { return true; }
  bool implementsAudioPorts() const noexcept override { return true; }
  std::int32_t getParamIndexForParamId(clap_id param_id) const noexcept override;
  bool getParamInfoForParamId(clap_id param_id, clap_param_info* info) const noexcept override;

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

  void deactivate() noexcept override { _engine.deactivate();  }
  clap_process_status process(clap_process const* process) noexcept override;
  bool activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept override;

  virtual bool implementsGui() const noexcept { return false; }
  virtual bool guiIsApiSupported(const char* api, bool isFloating) noexcept { return false; }
  virtual bool guiGetPreferredApi(const char** api, bool* is_floating) noexcept {
    return false;
  }
  virtual bool guiCreate(const char* api, bool isFloating) noexcept { return false; }
  virtual void guiDestroy() noexcept {}
  virtual bool guiSetScale(double scale) noexcept { return false; }
  virtual bool guiShow() noexcept { return false; }
  virtual bool guiHide() noexcept { return false; }
  virtual bool guiGetSize(uint32_t* width, uint32_t* height) noexcept { return false; }
  virtual bool guiCanResize() const noexcept { return false; }
  virtual bool guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept { return false; }
  virtual bool guiAdjustSize(uint32_t* width, uint32_t* height) noexcept {
    return guiGetSize(width, height);
  }
  virtual bool guiSetSize(uint32_t width, uint32_t height) noexcept { return false; }
  virtual void guiSuggestTitle(const char* title) noexcept {}
  virtual bool guiSetParent(const clap_window* window) noexcept { return false; }
  virtual bool guiSetTransient(const clap_window* window) noexcept { return false; }
};

}
