#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base.clap/pb_plugin.hpp>
#if (defined __linux__) || (defined  __FreeBSD__)
#include <juce_events/native/juce_EventLoopInternal_linux.h>
#endif

#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hxx>

#include <stack>
#include <vector>
#include <cstring>
#include <utility>
#include <algorithm>

using namespace juce;
using namespace moodycamel;
using namespace plugin_base;

namespace plugin_base::clap {

static int const midi_msg_cc = 176;
static int const midi_msg_cp = 208;
static int const midi_msg_pb = 224;
static int const midi_note_on = 0x90;
static int const midi_note_off = 0x80;

static normalized_value
clap_to_normalized(param_topo const& topo, clap_value clap)
{
  if(topo.domain.is_real())
    return normalized_value(clap.value());
  else
    return normalized_value(topo.domain.raw_to_normalized(clap.value()));
}

static clap_value
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
  auto plugin = static_cast<pb_plugin*>(context);
  return plugin->thread_pool_voice_processor(engine);
}

pb_plugin::
~pb_plugin() 
{ 
  stopTimer();
  _gui_state.remove_any_listener(this);
}

pb_plugin::
pb_plugin(
  clap_plugin_descriptor const* clap_desc, 
  clap_host const* host, plugin_topo const* topo):
Plugin(clap_desc, host), 
_desc(std::make_unique<plugin_desc>(topo, this)),
_engine(_desc.get(), topo->audio_polyphony, forward_thread_pool_voice_processor, this),
_extra_state(gui_extra_state_keyset(*_desc->plugin)), _gui_state(_desc.get(), true),
_to_gui_events(std::make_unique<event_queue>(default_q_size)), 
_to_audio_events(std::make_unique<event_queue>(default_q_size))
{ 
  _gui_state.add_any_listener(this);
  _block_automation_seen.resize(_engine.state().desc().param_count); 
}

void
pb_plugin::gui_param_begin_changes(int index)
{
  _gui_state.begin_undo_region();
  push_to_audio(index, sync_event_type::begin_edit);
}

void
pb_plugin::gui_param_end_changes(int index) 
{ 
  push_to_audio(index, sync_event_type::end_edit); 
  _gui_state.end_undo_region("Change " + _gui_state.desc().params[index]->full_name);
}

void
pb_plugin::param_state_changed(int index, plain_value plain)
{
  if (_gui_state.desc().params[index]->param->dsp.direction == param_direction::output) return;
  push_to_audio(index, plain);
  _gui_state.set_plain_at_index(index, plain);
}

bool
pb_plugin::init() noexcept
{
  // Need to start timer on the main thread. 
  // Constructor is not guaranteed to run there.
  startTimerHz(60);
  return true;
}

void 
pb_plugin::timerCallback()
{
  sync_event e;
  while (_to_gui_events->try_dequeue(e))
    _gui_state.set_plain_at_index(e.index, e.plain);
}

bool 
pb_plugin::stateSave(clap_ostream const* stream) noexcept
{
  std::vector<char> data(plugin_io_save_all(_gui_state, _extra_state));
  return stream->write(stream, data.data(), data.size()) == data.size();
}

bool 
pb_plugin::stateLoad(clap_istream const* stream) noexcept
{
  std::vector<char> data;
  do {
    char byte;
    int read = stream->read(stream, &byte, 1);
    if (read == 0) break;
    if (read < 0 || read > 1) return false;
    data.push_back(byte);
  } while(true);

  _gui_state.begin_undo_region();
  if (!plugin_io_load_all(data, _gui_state, _extra_state).ok()) 
  {
    _gui_state.discard_undo_region();
    return false;
  }
  for (int p = 0; p < _engine.state().desc().param_count; p++)
    gui_param_changed(p, _gui_state.get_plain_at_index(p));
  _gui_state.discard_undo_region();
  _engine.mark_all_params_as_automated(true);
  return true;
}

bool
pb_plugin::guiShow() noexcept
{
  _gui->setVisible(true);
  return true;
}

bool
pb_plugin::guiHide() noexcept
{
  _gui->setVisible(false);
  return true;
}

#if (defined __linux__) || (defined  __FreeBSD__)
void
pb_plugin::onPosixFd(int fd, int flags) noexcept
{ LinuxEventLoopInternal::invokeEventLoopCallbackForFd(fd); }
#endif

void 
pb_plugin::guiDestroy() noexcept
{
  _gui->remove_param_listener(this);
  _gui->setVisible(false);
  _gui->removeFromDesktop();
  _gui.reset();
#if (defined __linux__) || (defined  __FreeBSD__)
  for (int fd : LinuxEventLoopInternal::getRegisteredFds())
    _host.posixFdSupportUnregister(fd);
#endif
}

bool
pb_plugin::guiSetParent(clap_window const* window) noexcept
{
  _gui->addToDesktop(0, window->ptr);
#if (defined __linux__) || (defined  __FreeBSD__)
  for (int fd : LinuxEventLoopInternal::getRegisteredFds())
    _host.posixFdSupportRegister(fd, CLAP_POSIX_FD_READ);
#endif
  _gui->setVisible(true);
  _gui->add_param_listener(this);
  _gui->reloaded();
  return true;
}

bool
pb_plugin::guiIsApiSupported(char const* api, bool is_floating) noexcept
{
  if (is_floating) return false;
#if(WIN32)
  return !strcmp(api, CLAP_WINDOW_API_WIN32);
#elif (defined __linux__) || (defined  __FreeBSD__)
  return _host.canUsePosixFdSupport() && !strcmp(api, CLAP_WINDOW_API_X11);
#else
#error
#endif
}

bool
pb_plugin::guiSetSize(uint32_t width, uint32_t height) noexcept
{
  guiAdjustSize(&width, &height);
  _gui->setSize(width, height);
  return true;
}

bool
pb_plugin::guiCreate(char const* api, bool is_floating) noexcept
{
  _gui = std::make_unique<plugin_gui>(&_gui_state, &_extra_state);
  return true;
}

bool
pb_plugin::guiGetSize(uint32_t* width, uint32_t* height) noexcept
{
  *width = _gui->getWidth();
  *height = _gui->getHeight();
  guiAdjustSize(width, height);
  return true;
}

bool
pb_plugin::guiAdjustSize(uint32_t* width, uint32_t* height) noexcept
{
  auto const& topo = *_engine.state().desc().plugin;
  *width = std::max((int)*width, topo.gui.min_width);
  *height = *width * topo.gui.aspect_ratio_height / topo.gui.aspect_ratio_width;
  return true;
}

bool
pb_plugin::guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept
{
  hints->preserve_aspect_ratio = true;
  hints->can_resize_vertically = true;
  hints->can_resize_horizontally = true;
  hints->aspect_ratio_width = _engine.state().desc().plugin->gui.aspect_ratio_width;
  hints->aspect_ratio_height = _engine.state().desc().plugin->gui.aspect_ratio_height;
  return true;
}

void 
pb_plugin::push_to_audio(int index, plain_value plain)
{
  sync_event e;
  e.plain = plain;
  e.index = index;
  e.type = sync_event_type::value_changing;
  _to_audio_events->enqueue(e);
}

void 
pb_plugin::push_to_audio(int index, sync_event_type type)
{
  sync_event e;
  e.type = type;
  e.index = index;
  _to_audio_events->enqueue(e);
}

void
pb_plugin::push_to_gui(int index, clap_value clap)
{
  sync_event e;
  auto const& topo = *_engine.state().desc().param_at_index(index).param;
  e.index = index;
  e.type = sync_event_type::value_changing;
  e.plain = topo.domain.normalized_to_plain(clap_to_normalized(topo, clap));
  _to_gui_events->enqueue(e);
}

std::int32_t
pb_plugin::getParamIndexForParamId(clap_id param_id) const noexcept
{
  auto iter = _engine.state().desc().param_mappings.tag_to_index.find(param_id);
  if (iter == _engine.state().desc().param_mappings.tag_to_index.end())
  {
    assert(false);
    return -1;
  }
  return iter->second;
}

bool 
pb_plugin::getParamInfoForParamId(clap_id param_id, clap_param_info* info) const noexcept
{
  std::int32_t index = getParamIndexForParamId(param_id);
  if(index == -1) return false;
  return paramsInfo(index, info);
} 

bool
pb_plugin::paramsValue(clap_id param_id, double* value) noexcept
{
  auto const& topo = *_gui_state.desc().param_at_tag(param_id).param;
  auto normalized = _gui_state.get_normalized_at_tag(param_id);
  *value = normalized_to_clap(topo, normalized).value();
  return true;
}

bool
pb_plugin::paramsTextToValue(clap_id param_id, char const* display, double* value) noexcept
{
  normalized_value normalized;
  int index = _engine.state().desc().param_mappings.tag_to_index.at(param_id);
  auto const& param = *_engine.state().desc().param_at_tag(param_id).param;
  if (!_engine.state().text_to_normalized_at_index(false, index, display, normalized)) return false;
  *value = normalized_to_clap(param, normalized).value();
  return true;
}

bool
pb_plugin::paramsValueToText(clap_id param_id, double value, char* display, std::uint32_t size) noexcept
{
  int index = _engine.state().desc().param_mappings.tag_to_index.at(param_id);
  auto const& param = *_engine.state().desc().param_at_tag(param_id).param;
  normalized_value normalized = clap_to_normalized(param, clap_value(value));
  std::string text = _engine.state().normalized_to_text_at_index(false, index, normalized);
  from_8bit_string(display, size, text.c_str());
  return true;
}

bool
pb_plugin::paramsInfo(std::uint32_t index, clap_param_info* info) const noexcept
{
  param_desc const& param = _engine.state().desc().param_at_index(index);
  param_mapping const& mapping(_engine.state().desc().param_mappings.params[index]);
  module_desc const& module = _engine.state().desc().modules[mapping.module_global];
  
  info->cookie = nullptr;
  info->id = param.info.id_hash;
  from_8bit_string(info->name, param.full_name.c_str());
  from_8bit_string(info->module, module.info.name.c_str());

  info->flags = 0;
  if(!param.param->dsp.can_automate(module.info.slot))
    info->flags |= CLAP_PARAM_IS_READONLY;
  else
  {
    info->flags |= CLAP_PARAM_IS_AUTOMATABLE;
    info->flags |= CLAP_PARAM_REQUIRES_PROCESS;
  }

  // this is what clap_value is for
  auto default_normalized = param.param->domain.default_normalized(module.info.slot, param.info.slot);
  info->default_value = normalized_to_clap(*param.param, default_normalized).value();
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
pb_plugin::paramsFlush(clap_input_events const* in, clap_output_events const* out) noexcept
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
pb_plugin::notePortsInfo(std::uint32_t index, bool is_input, clap_note_port_info* info) const noexcept
{
  if (!is_input || index != 0) return false;
  info->id = 0;
  info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
  info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
  return true;
}

std::uint32_t 
pb_plugin::audioPortsCount(bool is_input) const noexcept
{
  if (!is_input) return 1;
  return _engine.state().desc().plugin->type == plugin_type::fx? 1: 0;
}

bool
pb_plugin::audioPortsInfo(std::uint32_t index, bool is_input, clap_audio_port_info* info) const noexcept
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
pb_plugin::activate(double sample_rate, std::uint32_t min_frame_count, std::uint32_t max_frame_count) noexcept
{
  _engine.activate(max_frame_count);
  _engine.set_sample_rate(sample_rate);
  _engine.activate_modules();
  return true;
}

void 
pb_plugin::process_gui_to_audio_events(const clap_output_events_t* out)
{
  sync_event e;
  while (_to_audio_events->try_dequeue(e))
  {
    int tag = _engine.state().desc().param_mappings.index_to_tag[e.index];
    auto m = _engine.state().desc().param_mappings.params[e.index].topo;
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
      _engine.mark_param_as_automated(m.module_index, m.module_slot, m.param_index, m.param_slot);
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

std::unique_ptr<host_menu>
pb_plugin::context_menu(int param_id) const
{
  if (!_host.canUseContextMenu()) return {};

  clap_context_menu_builder builder;
  auto result = std::make_unique<host_menu>();
  std::stack<host_menu_item*> menu_stack;
  menu_stack.push(&result->root);
  builder.ctx = &menu_stack;
  builder.supports = [](auto, auto) { return true; };

  builder.add_item = [](auto const* builder, auto kind, void const* data) {

    clap_context_menu_entry const* entry;
    clap_context_menu_item_title const* title;
    clap_context_menu_submenu const* sub_menu;
    clap_context_menu_check_entry const* check_entry;
    auto menu_stack = static_cast<std::stack<host_menu_item*>*>(builder->ctx);
    auto item = menu_stack->top()->children.emplace_back(std::make_shared<host_menu_item>());
    item->tag = host_menu_item::no_tag;

    switch (kind)
    {
    case CLAP_CONTEXT_MENU_ITEM_SEPARATOR:
      item->flags = host_menu_flags_separator;
      break;
    case CLAP_CONTEXT_MENU_ITEM_TITLE:
      title = static_cast<clap_context_menu_item_title const*>(data);
      item->name = title->title;
      item->flags |= title->is_enabled ? host_menu_flags_enabled : 0;
      break;
    case CLAP_CONTEXT_MENU_ITEM_ENTRY:
      entry = static_cast<clap_context_menu_entry const*>(data);
      item->name = entry->label;
      item->tag = entry->action_id;
      item->flags |= entry->is_enabled ? host_menu_flags_enabled : 0;
      break;
    case CLAP_CONTEXT_MENU_ITEM_CHECK_ENTRY:
      check_entry = static_cast<clap_context_menu_check_entry const*>(data);
      item->name = check_entry->label;
      item->tag = check_entry->action_id;
      item->flags |= check_entry->is_enabled ? host_menu_flags_enabled : 0;
      item->flags |= check_entry->is_checked ? host_menu_flags_checked : 0;
      break;
    case CLAP_CONTEXT_MENU_ITEM_BEGIN_SUBMENU:
      sub_menu = static_cast<clap_context_menu_submenu const*>(data);
      item->name = sub_menu->label;
      item->flags |= sub_menu->is_enabled? host_menu_flags_enabled: 0;
      menu_stack->push(item.get());
      break;
    case CLAP_CONTEXT_MENU_ITEM_END_SUBMENU:
      menu_stack->pop();
      break;
    default:
      assert(false);
    }

    return true;
  };

  auto target = std::make_unique<clap_context_menu_target>();
  target->id = param_id;
  target->kind = CLAP_CONTEXT_MENU_TARGET_KIND_PARAM;
  // bitwig returns false
  _host.contextMenuPopulate(_host.host(), target.get(), &builder);
  assert(menu_stack.size() == 1);
  menu_stack.pop();

  if(result->root.children.empty()) return {};
  result->clicked = [this, target = target.release()](int action) {
    _host.contextMenuPerform(_host.host(), target, action);
    delete target;
  };
  return result;
}

clap_process_status
pb_plugin::process(clap_process const* process) noexcept
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
    float midi_pb_value = 0;
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
      note.frame = header->time;
      note.id.channel = event->channel;
      block.events.notes.push_back(note);
      break;
    }
    case CLAP_EVENT_PARAM_VALUE:
    {
      auto event = reinterpret_cast<clap_event_param_value const*>(header);
      int index = getParamIndexForParamId(event->param_id);
      auto const& param = _engine.state().desc().param_at_index(index);
      push_to_gui(index, clap_value(event->value));
      if (param.param->dsp.rate != param_rate::accurate)
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
    case CLAP_EVENT_MIDI:
    {
      auto event = reinterpret_cast<clap_event_midi const*>(header);
      if(event->port_index != 0) continue;
      int message = event->data[0] & 0xF0;
      midi_event midi_event = {};
      midi_event.frame = header->time;
      switch (message)
      {
      case midi_msg_cp: 
        midi_event.id = midi_source_cp;
        midi_event.normalized = normalized_value(event->data[1] / 127.0);
        block.events.midi.push_back(midi_event);
        break;
      case midi_msg_cc:
        midi_event.id = event->data[1];
        midi_event.normalized = normalized_value(event->data[2] / 127.0);
        block.events.midi.push_back(midi_event);
        break;
      case midi_msg_pb:
        midi_event.id = midi_source_pb;
        midi_pb_value = ((event->data[2] << 7) | event->data[1]) / static_cast<double>(1U << 14);
        midi_event.normalized = normalized_value(midi_pb_value);
        block.events.midi.push_back(midi_event);
        break;
      // map these to note events
      case midi_note_on:
      case midi_note_off:
      {
        note_event note = {};
        note.frame = header->time;
        note.id.key = event->data[1];
        note.velocity = event->data[2] / 127.0f;
        note.type = message == midi_note_on ? note_event_type::on : note_event_type::off;
        block.events.notes.push_back(note);
        break;
      }
      default:
        break;
      }
      break;
    }
    default: 
      break;
    }
  }

  _engine.process();
  for (int e = 0; e < block.events.out.size(); e++)
  {
    sync_event to_gui_event = {};
    auto const& out_event = block.events.out[e];
    to_gui_event.index = out_event.param;
    to_gui_event.plain =  _engine.state().desc().normalized_to_plain_at_index(out_event.param, out_event.normalized);
    _to_gui_events->enqueue(to_gui_event);
  }
  _engine.release_block();
  return CLAP_PROCESS_CONTINUE;
}

}
