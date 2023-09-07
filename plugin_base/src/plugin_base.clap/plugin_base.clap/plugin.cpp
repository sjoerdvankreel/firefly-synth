#include <plugin_base.clap/plugin.hpp>
#include <plugin_base/param_value.hpp>
#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hxx>
#include <vector>
#include <atomic>
#include <utility>

using namespace plugin_base;

namespace plugin_base::clap {

plugin::
plugin(clap_plugin_descriptor const* desc, clap_host const* host, plugin_topo_factory factory):
Plugin(desc, host), _engine(factory)
{
  plugin_dims dims(_engine.desc().topo);
  _ui_state.init(dims.module_param_counts);
  _engine.desc().init_default_state(_ui_state);
  _block_automation_seen.resize(_engine.desc().param_mappings.size());
}

std::int32_t 
plugin::getParamIndexForParamId(clap_id param_id) const noexcept
{
  auto iter = _engine.desc().id_to_index.find(param_id);
  if (iter == _engine.desc().id_to_index.end())
  {
    assert(false);
    return -1;
  }
  return iter->second;
}

bool 
plugin::getParamInfoForParamId(clap_id param_id, clap_param_info* info) const noexcept
{
  std::int32_t index = getParamIndexForParamId(param_id);
  if(index == -1) return false;
  return paramsInfo(index, info);
} 

bool
plugin::paramsValue(clap_id param_id, double* value) noexcept
{
  // if audio thread set new values, make sure we pick them up
  // note: this may be called a *lot* but guaranteed on ui-thread only
  std::atomic_thread_fence(std::memory_order_acquire);
  int param_index = getParamIndexForParamId(param_id);
  param_mapping mapping(_engine.desc().param_mappings[param_index]);
  *value = mapping.value_at(_ui_state).to_plain(*_engine.desc().param_at(mapping).topo);
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
  param_value base_value;
  int param_index = getParamIndexForParamId(param_id);
  param_mapping mapping(_engine.desc().param_mappings[param_index]);
  auto const& param = *_engine.desc().param_at(mapping).topo;
  if(!param_value::from_text(param, display, base_value)) return false;
  *value = base_value.to_plain(param);
  return true;
}

bool
plugin::paramsValueToText(clap_id param_id, double value, char* display, std::uint32_t size) noexcept
{
  int param_index = getParamIndexForParamId(param_id);
  param_mapping mapping(_engine.desc().param_mappings[param_index]);
  auto const& param = *_engine.desc().param_at(mapping).topo;
  std::string text = param_value::from_plain(param, value).to_text(param);
  from_8bit_string(display, size, text.c_str());
  return false;
}

void
plugin::paramsFlush(clap_input_events const* in, clap_output_events const* out) noexcept
{
  for (std::uint32_t i = 0; i < in->size(in); i++)
  {
    auto header = in->get(in, i);
    if (header->type != CLAP_EVENT_PARAM_VALUE) continue;
    if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
    auto event = reinterpret_cast<clap_event_param_value const*>(header);
    int index = getParamIndexForParamId(event->param_id);
    auto mapping = _engine.desc().param_mappings[index];
    mapping.value_at(_ui_state) = param_value::from_plain(*_engine.desc().param_at(mapping).topo, event->value);
    mapping.value_at(_engine.state()) = mapping.value_at(_ui_state);
  }

  // we just set new values, make sure the other thread picks them up
  // note: this may be called on *either* the audio or ui thread!
  if(in->size(in) != 0) 
    std::atomic_thread_fence(std::memory_order_release);
}

bool
plugin::notePortsInfo(std::uint32_t index, bool is_input, clap_note_port_info* info) const noexcept
{
  if (!is_input || index != 0) return false;
  info->id = 0;
  info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
  info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
  return true;
}

std::uint32_t 
plugin::audioPortsCount(bool is_input) const noexcept
{
  if (!is_input) return 1;
  return _engine.desc().topo.type == plugin_type::fx? 1: 0;
}

bool
plugin::audioPortsInfo(std::uint32_t index, bool is_input, clap_audio_port_info* info) const noexcept
{
  if (index != 0) return false;
  if (is_input && _engine.desc().topo.type == plugin_type::synth) return false;
  info->id = 0;
  info->channel_count = 2;
  info->port_type = CLAP_PORT_STEREO;
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->in_place_pair = CLAP_INVALID_ID;
  return true;
}

bool
plugin::activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept
{
  _engine.activate(sample_rate, max_frame_count);
  return true;
}

clap_process_status
plugin::process(clap_process const* process) noexcept
{
  host_block& block = _engine.prepare();
  block.common->frame_count = process->frames_count;
  block.common->stream_time = process->steady_time;
  block.common->bpm = process->transport? process->transport->tempo: 0;
  block.common->audio_input = process->audio_inputs? process->audio_inputs[0].data32: nullptr;
  block.common->audio_output = process->audio_outputs[0].data32;

  // if ui thread set new values, make sure we pick them up
  std::atomic_thread_fence(std::memory_order_acquire);

  // make sure we only push per-block events at most 1 time
  std::fill(_block_automation_seen.begin(), _block_automation_seen.end(), 0);

  for (std::uint32_t i = 0; i < process->in_events->size(process->in_events); i++)
  {
    auto header = process->in_events->get(process->in_events, i);
    if(header->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
    switch (header->type)
    {
    case CLAP_EVENT_NOTE_ON:
    case CLAP_EVENT_NOTE_OFF:
    case CLAP_EVENT_NOTE_CHOKE:
    {
      note_event note = {};
      auto event = reinterpret_cast<clap_event_note_t const*>(header);
      if (header->type == CLAP_EVENT_NOTE_ON) note.type = note_event_type::on;
      else if (header->type == CLAP_EVENT_NOTE_OFF) note.type = note_event_type::off;
      else if (header->type == CLAP_EVENT_NOTE_CHOKE) note.type = note_event_type::cut;
      note.id.key = event->key;
      note.id.id = event->note_id;
      note.velocity = event->velocity;
      note.frame_index = header->time;
      note.id.channel = event->channel;
      block.common->notes.push_back(note);
    }
    case CLAP_EVENT_PARAM_VALUE:
    {
      auto event = reinterpret_cast<clap_event_param_value const*>(header);
      int param_index = getParamIndexForParamId(event->param_id);
      auto mapping = _engine.desc().param_mappings[param_index];
      auto const& param = _engine.desc().param_at(mapping);
      if (param.topo->rate == param_rate::block)
      {
        if (_block_automation_seen[param_index] == 0)
        {
          host_block_event block_event;
          block_event.plugin_param_index = param_index;
          block_event.normalized = param_value::from_plain(*param.topo, event->value).to_normalized(*param.topo);
          block.block_events.push_back(block_event);
          _block_automation_seen[param_index] = 1;
        }
      } else {
        host_accurate_event accurate_event;
        accurate_event.frame_index = header->time;
        accurate_event.plugin_param_index = param_index;
        accurate_event.normalized = param_value::from_plain(*param.topo, event->value).to_normalized(*param.topo);
        block.accurate_events.push_back(accurate_event);
      }
    }
    default: break;
    }
  }

  _engine.process();

  // by now the audio engine state may have been changed due to events or output parameters
  // we need to copy everything back onto the ui thread and flush from audio to ui
  for (int i = 0; i < _engine.desc().param_mappings.size(); i++)
  {
    auto mapping = _engine.desc().param_mappings[i];
    mapping.value_at(_ui_state) = mapping.value_at(_engine.state());
  }
  std::atomic_thread_fence(std::memory_order_release);
  return CLAP_PROCESS_CONTINUE;
}

}
