#pragma once
#include <plugin_base/gui.hpp>
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
  std::unique_ptr<plugin_gui> _gui = {};
  plugin_topo_factory const _topo_factory = {};
  jarray3d<param_value> _ui_state = {}; // Copy of engine state on the ui thread.
  std::vector<int> _block_automation_seen = {}; // Only push the first event in per-block automation.
public:
  plugin(clap_plugin_descriptor const* desc, clap_host const* host, plugin_topo_factory factory);
  
  bool implementsGui() const noexcept override { return true; }
  bool implementsParams() const noexcept override { return true; }
  bool implementsNotePorts() const noexcept override { return true; }
  bool implementsAudioPorts() const noexcept override { return true; }

  std::int32_t getParamIndexForParamId(clap_id param_id) const noexcept override;
  bool getParamInfoForParamId(clap_id param_id, clap_param_info* info) const noexcept override;

  bool guiShow() noexcept override;
  bool guiHide() noexcept override;
  bool guiSetParent(clap_window const* window) noexcept override;
  bool guiCreate(char const* api, bool is_floating) noexcept override;
  bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
  bool guiGetSize(uint32_t* width, uint32_t* height) noexcept override;
  bool guiAdjustSize(uint32_t* width, uint32_t* height) noexcept override;
  bool guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept override;
  void guiDestroy() noexcept override { _gui.reset(); }
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

  void deactivate() noexcept override { _engine.deactivate();  }
  clap_process_status process(clap_process const* process) noexcept override;
  bool activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept override;
};

}
