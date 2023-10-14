#include <plugin_base/shared/io.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base.clap/inf_plugin.hpp>
#if (defined __linux__) || (defined  __FreeBSD__)
#include <juce_events/native/juce_EventLoopInternal_linux.h>
#endif

#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hxx>

#include <vector>
#include <cstring>
#include <utility>
#include <algorithm>

using namespace juce;
using namespace moodycamel;
using namespace plugin_base;

namespace plugin_base::clap {

static inline normalized_value
clap_to_normalized(param_topo const& topo, clap_value clap)
{
  if(topo.domain.is_real())
    return normalized_value(clap.value());
  else
    return normalized_value(topo.domain.raw_to_normalized(clap.value()));
}

static inline clap_value
normalized_to_clap(param_topo const& topo, normalized_value normalized)
{
  if(topo.domain.is_real())
    return clap_value(normalized.value());
  else
    return clap_value(topo.domain.normalized_to_raw(normalized));
}

static bool
forward_thread_pool_voice_processor(plugin_engine& engine, void* context)
{
  auto plugin = static_cast<inf_plugin*>(context);
  return plugin->thread_pool_voice_processor(engine);
}

inf_plugin::
inf_plugin(
  clap_plugin_descriptor const* clap_desc, 
  clap_host const* host, plugin_desc const* desc):
Plugin(clap_desc, host), 
_engine(desc, forward_thread_pool_voice_processor, this), _gui_state(desc),
_to_gui_events(std::make_unique<event_queue>(default_q_size)), 
_to_audio_events(std::make_unique<event_queue>(default_q_size))
{ _block_automation_seen.resize(_engine.state().desc().param_count); }

bool
inf_plugin::init() noexcept
{
  // Need to start timer on the main thread. 
  // Constructor is not guaranteed to run there.
  startTimerHz(60);
  return true;
}

void 
inf_plugin::timerCallback()
{
  sync_event e;
  while (_to_gui_events->try_dequeue(e))
  {
    _gui_state.set_plain_at_index(e.index, e.plain);
    if(_gui) _gui->plugin_changed(e.index, e.plain);
  }
}

bool 
inf_plugin::stateSave(clap_ostream const* stream) noexcept
{
  std::vector<char> data(plugin_io_save(_gui_state));
  return stream->write(stream, data.data(), data.size()) == data.size();
}

bool 
inf_plugin::stateLoad(clap_istream const* stream) noexcept
{
  std::vector<char> data;
  do {
    char byte;
    int read = stream->read(stream, &byte, 1);
    if (read == 0) break;
    if (read < 0 || read > 1) return false;
    data.push_back(byte);
  } while(true);

  if (!plugin_io_load(data, _gui_state).ok()) return false;
  for (int p = 0; p < _engine.state().desc().param_count; p++)
    gui_changed(p, _gui_state.get_plain_at_index(p));
  return true;
}

bool
inf_plugin::guiShow() noexcept
{
  _gui->setVisible(true);
  return true;
}

bool
inf_plugin::guiHide() noexcept
{
  _gui->setVisible(false);
  return true;
}

bool 
inf_plugin::guiSetScale(double scale) noexcept
{
  _gui->content_scale(scale);
  return true;
}

bool 
inf_plugin::guiIsApiSupported(char const* api, bool is_floating) noexcept
{
  if(is_floating) return false;
#if(WIN32)
  return !strcmp(api, CLAP_WINDOW_API_WIN32);
#elif (defined __linux__) || (defined  __FreeBSD__)
  return _host.canUsePosixFdSupport() && !strcmp(api, CLAP_WINDOW_API_X11);
#else
#error
#endif
}

bool
inf_plugin::guiSetParent(clap_window const* window) noexcept
{
  _gui->addToDesktop(0, window->ptr);
#if (defined __linux__) || (defined  __FreeBSD__)
  for (int fd : LinuxEventLoopInternal::getRegisteredFds())
    _host.posixFdSupportRegister(fd, CLAP_POSIX_FD_READ);
#endif
  _gui->setVisible(true);
  _gui->add_gui_listener(this);
  _gui->resized();
  return true;
}

#if (defined __linux__) || (defined  __FreeBSD__)
void
inf_plugin::onPosixFd(int fd, int flags) noexcept
{ LinuxEventLoopInternal::invokeEventLoopCallbackForFd(fd); }
#endif

void 
inf_plugin::guiDestroy() noexcept
{
  _gui->remove_gui_listener(this);
  _gui->setVisible(false);
  _gui->removeFromDesktop();
  _gui.reset();
#if (defined __linux__) || (defined  __FreeBSD__)
  for (int fd : LinuxEventLoopInternal::getRegisteredFds())
    _host.posixFdSupportUnregister(fd);
#endif
}

bool
inf_plugin::guiCreate(char const* api, bool is_floating) noexcept
{
  _gui = std::make_unique<plugin_gui>(&_gui_state);
  return true;
}

bool
inf_plugin::guiSetSize(uint32_t width, uint32_t height) noexcept
{
  guiAdjustSize(&width, &height);
  _gui->setSize(width, height);
  return true;
}

bool
inf_plugin::guiGetSize(uint32_t* width, uint32_t* height) noexcept
{
  *width = _gui->getWidth();
  *height = _gui->getHeight();
  guiAdjustSize(width, height);
  return true;
}

bool
inf_plugin::guiAdjustSize(uint32_t* width, uint32_t* height) noexcept
{
  auto const& topo = *_engine.state().desc().plugin;
  *width = std::clamp((int)*width, topo.gui.min_width, topo.gui.max_width);
  *height = *width * topo.gui.aspect_ratio_height / topo.gui.aspect_ratio_width;
  return true;
}

bool
inf_plugin::guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept
{
  hints->preserve_aspect_ratio = true;
  hints->can_resize_vertically = true;
  hints->can_resize_horizontally = true;
  hints->aspect_ratio_width = _engine.state().desc().plugin->gui.aspect_ratio_width;
  hints->aspect_ratio_height = _engine.state().desc().plugin->gui.aspect_ratio_height;
  return true;
}

void 
inf_plugin::gui_changing(int index, plain_value plain)
{ 
  push_to_audio(index, plain);
  _gui_state.set_plain_at_index(index, plain);
  if(_gui) _gui->plugin_changed(index, plain);
}

void 
inf_plugin::push_to_audio(int index, plain_value plain)
{
  sync_event e;
  e.plain = plain;
  e.index = index;
  e.type = sync_event_type::value_changing;
  _to_audio_events->enqueue(e);
}

void 
inf_plugin::push_to_audio(int index, sync_event_type type)
{
  sync_event e;
  e.type = type;
  e.index = index;
  _to_audio_events->enqueue(e);
}

void
inf_plugin::push_to_gui(int index, clap_value clap)
{
  sync_event e;
  param_mapping const& mapping = _engine.state().desc().mappings.params[index];
  auto const& topo = *_engine.state().desc().param_at(mapping).param;
  e.index = index;
  e.type = sync_event_type::value_changing;
  e.plain = topo.domain.normalized_to_plain(clap_to_normalized(topo, clap));
  _to_gui_events->enqueue(e);
}

std::int32_t
inf_plugin::getParamIndexForParamId(clap_id param_id) const noexcept
{
  auto iter = _engine.state().desc().mappings.tag_to_index.find(param_id);
  if (iter == _engine.state().desc().mappings.tag_to_index.end())
  {
    assert(false);
    return -1;
  }
  return iter->second;
}

bool 
inf_plugin::getParamInfoForParamId(clap_id param_id, clap_param_info* info) const noexcept
{
  std::int32_t index = getParamIndexForParamId(param_id);
  if(index == -1) return false;
  return paramsInfo(index, info);
} 

bool
inf_plugin::paramsValue(clap_id param_id, double* value) noexcept
{
  auto const& topo = *_gui_state.desc().param_at_tag(param_id).param;
  auto normalized = _gui_state.get_normalized_at_tag(param_id);
  *value = normalized_to_clap(topo, normalized).value();
  return true;
}

bool
inf_plugin::paramsTextToValue(clap_id param_id, char const* display, double* value) noexcept
{
  normalized_value normalized;
  int index = getParamIndexForParamId(param_id);
  param_mapping const& mapping(_engine.state().desc().mappings.params[index]);
  auto const& param = *_engine.state().desc().param_at(mapping).param;
  if (!param.domain.text_to_normalized(display, normalized)) return false;
  *value = normalized_to_clap(param, normalized).value();
  return true;
}

bool
inf_plugin::paramsValueToText(clap_id param_id, double value, char* display, std::uint32_t size) noexcept
{
  int index = getParamIndexForParamId(param_id);
  param_mapping const& mapping(_engine.state().desc().mappings.params[index]);
  auto const& param = *_engine.state().desc().param_at(mapping).param;
  normalized_value normalized = clap_to_normalized(param, clap_value(value));
  std::string text = param.domain.normalized_to_text(normalized);
  from_8bit_string(display, size, text.c_str());
  return true;
}

bool
inf_plugin::paramsInfo(std::uint32_t index, clap_param_info* info) const noexcept
{
  param_mapping const& mapping(_engine.state().desc().mappings.params[index]);
  param_desc const& param = _engine.state().desc().param_at(mapping);
  info->cookie = nullptr;
  info->id = param.info.id_hash;
  from_8bit_string(info->name, _engine.state().desc().param_at(mapping).full_name.c_str());
  from_8bit_string(info->module, _engine.state().desc().modules[mapping.module_global].info.name.c_str());

  info->flags = 0;
  if(param.param->dsp.direction != param_direction::input)
    info->flags |= CLAP_PARAM_IS_READONLY;
  else
  {
    info->flags |= CLAP_PARAM_IS_AUTOMATABLE;
    info->flags |= CLAP_PARAM_REQUIRES_PROCESS;
  }

  // this is what clap_value is for
  info->default_value = normalized_to_clap(*param.param, param.param->domain.default_normalized()).value();
  if (param.param->domain.is_real())
  {
    info->min_value = 0;
    info->max_value = 1;
  }
  else
  {
    info->min_value = param.param->domain.min;
    info->max_value = param.param->domain.max;
    info->flags |= CLAP_PARAM_IS_STEPPED;
  }
  return true;
}

void
inf_plugin::paramsFlush(clap_input_events const* in, clap_output_events const* out) noexcept
{
  for (std::uint32_t i = 0; i < in->size(in); i++)
  {
    auto header = in->get(in, i);
    if (header->type != CLAP_EVENT_PARAM_VALUE) continue;
    if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
    auto event = reinterpret_cast<clap_event_param_value const*>(header);
    int index = getParamIndexForParamId(event->param_id);
    auto const& param = *_engine.state().desc().param_at_index(index).param;
    auto normalized = clap_to_normalized(param, clap_value(event->value));
    _engine.state().set_normalized_at_index(index, normalized);
    push_to_gui(index, clap_value(event->value));
  }
  process_gui_to_audio_events(out);
}

bool
inf_plugin::notePortsInfo(std::uint32_t index, bool is_input, clap_note_port_info* info) const noexcept
{
  if (!is_input || index != 0) return false;
  info->id = 0;
  info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
  info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
  return true;
}

std::uint32_t 
inf_plugin::audioPortsCount(bool is_input) const noexcept
{
  if (!is_input) return 1;
  return _engine.state().desc().plugin->type == plugin_type::fx? 1: 0;
}

bool
inf_plugin::audioPortsInfo(std::uint32_t index, bool is_input, clap_audio_port_info* info) const noexcept
{
  if (index != 0) return false;
  if (is_input && _engine.state().desc().plugin->type == plugin_type::synth) return false;
  info->id = 0;
  info->channel_count = 2;
  info->port_type = CLAP_PORT_STEREO;
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->in_place_pair = CLAP_INVALID_ID;
  return true;
}

bool
inf_plugin::activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept
{
  _engine.activate(sample_rate, max_frame_count);
  return true;
}

void 
inf_plugin::process_gui_to_audio_events(const clap_output_events_t* out)
{
  sync_event e;
  while (_to_audio_events->try_dequeue(e))
  {
    int tag = _engine.state().desc().mappings.index_to_tag[e.index];
    switch(e.type) 
    {
    case sync_event_type::value_changing:
    {
      auto event = clap_event_param_value();
      event.param_id = tag;
      event.header.time = 0;
      event.header.flags = 0;
      event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      event.header.size = sizeof(clap_event_param_value);
      event.header.type = (uint16_t)CLAP_EVENT_PARAM_VALUE;
      auto const& topo = *_engine.state().desc().param_at_index(e.index).param;
      event.value = normalized_to_clap(topo, topo.domain.plain_to_normalized(e.plain)).value();
      _engine.state().set_plain_at_index(e.index, e.plain);
      out->try_push(out, &(event.header));
      break;
    }
    case sync_event_type::end_edit:
    case sync_event_type::begin_edit:
    {
      auto event = clap_event_param_gesture();
      event.param_id = tag;
      event.header.time = 0;
      event.header.flags = 0;
      event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      event.header.size = sizeof(clap_event_param_gesture);
      event.header.type = (e.type == sync_event_type::begin_edit ? CLAP_EVENT_PARAM_GESTURE_BEGIN : CLAP_EVENT_PARAM_GESTURE_END);
      out->try_push(out, &event.header);
      break;
    }
    default: assert(false); break;
    }
  }
}

clap_process_status
inf_plugin::process(clap_process const* process) noexcept
{
  host_block& block = _engine.prepare_block();
  block.frame_count = process->frames_count;
  block.audio_out = process->audio_outputs[0].data32;
  block.shared.bpm = process->transport? process->transport->tempo: 0;
  block.shared.audio_in = process->audio_inputs? process->audio_inputs[0].data32: nullptr;

  process_gui_to_audio_events(process->out_events);

  // make sure we only push per-block events at most 1 time
  std::fill(_block_automation_seen.begin(), _block_automation_seen.end(), 0);

  for (std::uint32_t i = 0; i < process->in_events->size(process->in_events); i++)
  {
    auto header = process->in_events->get(process->in_events, i);
    if(header->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
    switch (header->type)
    {
    // TODO handle midi note events
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
      note.frame = header->time;
      note.id.channel = event->channel;
      block.events.notes.push_back(note);
      break;
    }
    case CLAP_EVENT_PARAM_VALUE:
    {
      auto event = reinterpret_cast<clap_event_param_value const*>(header);
      int index = getParamIndexForParamId(event->param_id);
      auto const& mapping = _engine.state().desc().mappings.params[index];
      auto const& param = _engine.state().desc().param_at(mapping);
      push_to_gui(index, clap_value(event->value));
      if (param.param->dsp.rate == param_rate::block)
      {
        if (_block_automation_seen[index] == 0)
        {
          block_event block_event;
          block_event.param = index;
          block_event.normalized = clap_to_normalized(*param.param, clap_value(event->value));
          block.events.block.push_back(block_event);
          _block_automation_seen[index] = 1;
        }
      } else {
        accurate_event accurate_event;
        accurate_event.frame = header->time;
        accurate_event.param = index;
        accurate_event.normalized = clap_to_normalized(*param.param, clap_value(event->value));
        block.events.accurate.push_back(accurate_event);
      }
      break;
    }
    default: break;
    }
  }

  _engine.process();
  for (int e = 0; e < block.events.out.size(); e++)
  {
    sync_event to_gui_event = {};
    auto const& out_event = block.events.out[e];
    auto const& mapping = _engine.state().desc().mappings.params[out_event.param];
    to_gui_event.index = out_event.param;
    to_gui_event.plain = _engine.state().desc().param_at(mapping).param->domain.normalized_to_plain(out_event.normalized);
    _to_gui_events->enqueue(to_gui_event);
  }
  _engine.release_block();
  return CLAP_PROCESS_CONTINUE;
}

}
