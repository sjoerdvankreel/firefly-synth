#include <infernal.base.clap/plugin.hpp>
#include <infernal.base/param_value.hpp>
#include <vector>
#include <utility>

using namespace infernal::base;

namespace infernal::base::clap {

plugin::
plugin(clap_plugin_descriptor const* desc, clap_host const* host, plugin_topo&& topo):
Plugin(desc, host), _engine(std::move(topo)) {}

std::int32_t 
plugin::getParamIndexForParamId(clap_id param_id) const noexcept
{
  auto iter = _engine.desc().id_to_index.find(param_id);
  return iter == _engine.desc().id_to_index.end()? -1: iter->second;
}

bool
plugin::paramsValue(clap_id param_id, double* value) noexcept
{
  int param_index = getParamIndexForParamId(param_id);
  param_mapping mapping(_engine.desc().param_mappings[param_index]);
  param_value base_value(_engine.state()[mapping.group][mapping.module][mapping.param]);
  *value = base_value.to_plain(_engine.topo().module_groups[mapping.group].params[mapping.param]);
  return true;
}

bool
plugin::paramsInfo(std::uint32_t param_index, clap_param_info* info) const noexcept
{
  param_mapping mapping(_engine.desc().param_mappings[param_index]);
  param_topo const& param = _engine.desc().modules[mapping.]
  info->cookie = nullptr;
  info->default_value = param_value::default_value(param).to_plain(param);
  info->id = _engine.desc().modules[mapping.group].params[]
  info-?>
}

bool
plugin::paramsTextToValue(clap_id param_id, char const* display, double* value) noexcept
{
}

void
plugin::paramsFlush(clap_input_events const* in, clap_output_events const* out) noexcept
{
}

bool
plugin::paramsValueToText(clap_id param_id, double value, char* display, std::uint32_t size) noexcept
{
}

std::uint32_t 
plugin::notePortsCount(bool is_input) const noexcept
{
}

bool
plugin::notePortsInfo(std::uint32_t index, bool is_input, clap_note_port_info* info) const noexcept
{
}

std::uint32_t 
plugin::audioPortsCount(bool is_input) const noexcept
{
}

bool
plugin::audioPortsInfo(std::uint32_t index, bool is_input, clap_audio_port_info* info) const noexcept
{
}

void 
plugin::deactivate() noexcept
{
}

clap_process_status
plugin::process(clap_process const* process) noexcept
{
}

bool
plugin::activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept
{
}

}
