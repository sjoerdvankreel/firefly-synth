#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

#include <map>
#include <vector>

namespace plugin_base {

enum class state_init_type { empty, minimal, default_ };

class state_listener
{
public:
  virtual void state_changed(int index, plain_value plain) = 0;
};

class any_state_listener
{
public:
  virtual void any_state_changed(int index, plain_value plain) = 0;
};

class plugin_state final {
  bool const _notify = {};
  jarray<plain_value, 4> _state = {};
  plugin_desc const* const _desc = {};
  std::vector<any_state_listener*> mutable _any_listeners = {};
  std::map<int, std::vector<state_listener*>> mutable _listeners = {};

  void state_changed(int index, plain_value plain) const;
  plain_value get_plain_at_mapping(param_topo_mapping const& m) const 
  { return get_plain_at(m.module_index, m.module_slot, m.param_index, m.param_slot); }
  void set_plain_at_mapping(param_topo_mapping const& m, plain_value value)
  { set_plain_at(m.module_index, m.module_slot, m.param_index, m.param_slot, value); }

public:
  plugin_state(plugin_desc const* desc, bool notify);
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_state);

  void init(state_init_type init_type);
  void add_any_listener(any_state_listener* listener) const;
  void remove_any_listener(any_state_listener* listener) const;
  void add_listener(int index, state_listener* listener) const;
  void remove_listener(int index, state_listener* listener) const;

  plugin_desc const& desc() const { return *_desc; }
  jarray<plain_value, 4> const& state() const { return _state; }

  void set_plain_at(int m, int mi, int p, int pi, plain_value value);
  plain_value get_plain_at(int m, int mi, int p, int pi) const
  { return _state[m][mi][p][pi]; }
  plain_value get_plain_at(param_topo_mapping m) const
  { return _state[m.module_index][m.module_slot][m.param_index][m.param_slot]; }
  plain_value get_plain_at_index(int index) const 
  { return get_plain_at_mapping(desc().param_mappings.params[index].topo); }
  void set_plain_at_index(int index, plain_value value) 
  { set_plain_at_mapping(desc().param_mappings.params[index].topo, value); }
  plain_value get_plain_at_tag(int tag) const 
  { return get_plain_at_index(desc().param_mappings.tag_to_index.at(tag)); }
  void set_plain_at_tag(int tag, plain_value value) 
  { set_plain_at_index(desc().param_mappings.tag_to_index.at(tag), value); }

  double get_raw_at(int m, int mi, int p, int pi) const 
  { return _desc->plain_to_raw_at(m, p, get_plain_at(m, mi, p, pi)); }
  void set_raw_at(int m, int mi, int p, int pi, double value)
  { set_plain_at(m, mi, p, pi, _desc->raw_to_plain_at(m, p, value)); }
  double get_raw_at_tag(int tag) const 
  { return get_raw_at_index(desc().param_mappings.tag_to_index.at(tag)); }
  void set_raw_at_tag(int tag, double value) 
  { set_raw_at_index(desc().param_mappings.tag_to_index.at(tag), value); }
  double get_raw_at_index(int index) const 
  { return desc().plain_to_raw_at_index(index, get_plain_at_index(index)); }
  void set_raw_at_index(int index, double value) 
  { set_plain_at_index(index, desc().raw_to_plain_at_index(index, value)); }

  void set_text_at(int m, int mi, int p, int pi, std::string const& value);
  normalized_value get_normalized_at(int m, int mi, int p, int pi) const 
  { return _desc->plain_to_normalized_at(m, p, get_plain_at(m, mi, p, pi)); }
  void set_normalized_at(int m, int mi, int p, int pi, normalized_value value)
  { set_plain_at(m, mi, p, pi,_desc->normalized_to_plain_at(m, p, value)); }
  normalized_value get_normalized_at_tag(int tag) const 
  { return get_normalized_at_index(desc().param_mappings.tag_to_index.at(tag)); }
  void set_normalized_at_tag(int tag, normalized_value value) 
  { set_normalized_at_index(desc().param_mappings.tag_to_index.at(tag), value); }
  normalized_value get_normalized_at_index(int index) const 
  { return desc().plain_to_normalized_at_index(index, get_plain_at_index(index)); }
  void set_normalized_at_index(int index, normalized_value value) 
  { set_plain_at_index(index, desc().normalized_to_plain_at_index(index, value)); }

  // parse and format
  std::string raw_to_text_at_index(bool io, int index, double raw) const
  { return plain_to_text_at_index(io, index, desc().raw_to_plain_at_index(index, raw)); }
  std::string normalized_to_text_at_index(bool io, int index, normalized_value normalized) const
  { return plain_to_text_at_index(io, index, desc().normalized_to_plain_at_index(index, normalized)); }
  bool text_to_plain_at_index(bool io, int index, std::string const& textual, plain_value& plain) const
  { return desc().param_at_index(index).param->domain.text_to_plain(io, textual, plain); }
  std::string plain_to_text_at_index(bool io, int index, plain_value plain) const
  { return desc().param_at_index(index).param->domain.plain_to_text(io, plain); }
  bool text_to_normalized_at_index(bool io, int index, std::string const& textual, normalized_value& normalized) const;
};

}