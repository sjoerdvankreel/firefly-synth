#include <plugin_base.clap/plugin.hpp>
#include <plugin_base/param_value.hpp>
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
  *value = mapping.value_at(_engine.state()).to_plain(*_engine.desc().param_at(mapping).topo);
  return true;
}

bool
plugin::paramsInfo(std::uint32_t param_index, clap_param_info* info) const noexcept
{
  param_mapping mapping(_engine.desc().param_mappings[param_index]);
  param_desc const& param = _engine.desc().param_at(mapping);
  info->cookie = nullptr;
  info->id = param.id_hash;
  info->min_value = param.topo->min;
  info->max_value = param.topo->max;
  info->default_value = param_value::default_value(*param.topo).to_plain(*param.topo);
  from_8bit_string(info->name, _engine.desc().param_at(mapping).name.c_str());
  from_8bit_string(info->module, _engine.desc().modules[mapping.module_in_plugin].name.c_str());

  info->flags = 0;
  if(param.topo->direction != param_direction::input) 
    info->flags |= CLAP_PARAM_IS_READONLY;
  else
  {
    info->flags |= CLAP_PARAM_IS_AUTOMATABLE;
    info->flags |= CLAP_PARAM_REQUIRES_PROCESS;
  }
  if(!param.topo->is_real())
    info->flags |= CLAP_PARAM_IS_STEPPED;
  return true;
}

bool
plugin::paramsTextToValue(clap_id param_id, char const* display, double* value) noexcept
{
  return false;
}

void
plugin::paramsFlush(clap_input_events const* in, clap_output_events const* out) noexcept
{
}

bool
plugin::paramsValueToText(clap_id param_id, double value, char* display, std::uint32_t size) noexcept
{
  return false;
}

std::uint32_t 
plugin::notePortsCount(bool is_input) const noexcept
{
  return 0;
}

bool
plugin::notePortsInfo(std::uint32_t index, bool is_input, clap_note_port_info* info) const noexcept
{
  return false;
}

std::uint32_t 
plugin::audioPortsCount(bool is_input) const noexcept
{
  return 0;
}

bool
plugin::audioPortsInfo(std::uint32_t index, bool is_input, clap_audio_port_info* info) const noexcept
{
  return false;
}

void 
plugin::deactivate() noexcept
{
}

clap_process_status
plugin::process(clap_process const* process) noexcept
{
  return 0;
}

bool
plugin::activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept
{
  return false;
}

}
