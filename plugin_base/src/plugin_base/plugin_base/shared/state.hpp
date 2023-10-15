#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

#include <map>
#include <vector>

namespace plugin_base {

class state_listener
{
public:
  virtual void state_changed(int index, plain_value plain) = 0;
};

class plugin_state final {
  bool const _notify = {};
  jarray<plain_value, 4> _state = {};
  plugin_desc const* const _desc = {};
  std::map<int, std::vector<state_listener*>> mutable _listeners = {};

  void state_changed(int index, plain_value plain) const;
  plain_value get_plain_at_mapping(param_mapping const& m) const 
  { return get_plain_at(m.module_topo, m.module_slot, m.param_topo, m.param_slot); }
  void set_plain_at_mapping(param_mapping const& m, plain_value value)
  { set_plain_at(m.module_topo, m.module_slot, m.param_topo, m.param_slot, value); }

public:
  plugin_state(plugin_desc const* desc, bool notify);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_state);

  void init_defaults();
  void add_listener(int index, state_listener* listener) const;
  void remove_listener(int index, state_listener* listener) const;

  plugin_desc const& desc() const { return *_desc; }
  jarray<plain_value, 2>& module_state_at(int module, int slot)
  { return _state[module][slot]; }
  jarray<plain_value, 2> const& module_state_at(int module, int slot) const
  { return _state[module][slot]; }

  void set_plain_at(int m, int mi, int p, int pi, plain_value value);
  plain_value get_plain_at(int m, int mi, int p, int pi) const 
  { return _state[m][mi][p][pi]; }
  plain_value get_plain_at_index(int index) const 
  { return get_plain_at_mapping(desc().mappings.params[index]); }
  void set_plain_at_index(int index, plain_value value) 
  { set_plain_at_mapping(desc().mappings.params[index], value); }
  plain_value get_plain_at_tag(int tag) const 
  { return get_plain_at_index(desc().mappings.tag_to_index.at(tag)); }
  void set_plain_at_tag(int tag, plain_value value) 
  { set_plain_at_index(desc().mappings.tag_to_index.at(tag), value); }

  double get_raw_at(int m, int mi, int p, int pi) const 
  { return _desc->plain_to_raw_at(m, p, get_plain_at(m, mi, p, pi)); }
  void set_raw_at(int m, int mi, int p, int pi, double value)
  { set_plain_at(m, mi, p, pi, _desc->raw_to_plain_at(m, p, value)); }
  double get_raw_at_tag(int tag) const 
  { return get_raw_at_index(desc().mappings.tag_to_index.at(tag)); }
  void set_raw_at_tag(int tag, double value) 
  { set_raw_at_index(desc().mappings.tag_to_index.at(tag), value); }
  double get_raw_at_index(int index) const 
  { return desc().plain_to_raw_at_index(index, get_plain_at_index(index)); }
  void set_raw_at_index(int index, double value) 
  { set_plain_at_index(index, desc().raw_to_plain_at_index(index, value)); }

  normalized_value get_normalized_at(int m, int mi, int p, int pi) const 
  { return _desc->plain_to_normalized_at(m, p, get_plain_at(m, mi, p, pi)); }
  void set_normalized_at(int m, int mi, int p, int pi, normalized_value value)
  { set_plain_at(m, mi, p, pi,_desc->normalized_to_plain_at(m, p, value)); }
  normalized_value get_normalized_at_tag(int tag) const 
  { return get_normalized_at_index(desc().mappings.tag_to_index.at(tag)); }
  void set_normalized_at_tag(int tag, normalized_value value) 
  { set_normalized_at_index(desc().mappings.tag_to_index.at(tag), value); }
  normalized_value get_normalized_at_index(int index) const 
  { return desc().plain_to_normalized_at_index(index, get_plain_at_index(index)); }
  void set_normalized_at_index(int index, normalized_value value) 
  { set_plain_at_index(index, desc().normalized_to_plain_at_index(index, value)); }
};

inline void 
plugin_state::set_plain_at(int m, int mi, int p, int pi, plain_value value)
{
  _state[m][mi][p][pi] = value;
  int index = desc().mappings.topo_to_index[m][mi][p][pi];
  if(_notify) state_changed(index, value);
  for (int d = 0; d < desc().param_dependents[index].size(); d++)
  {
    int dependent_index = desc().param_dependents[index][d];
    auto dependent_value = get_plain_at_index(dependent_index);
    auto clamped = desc().param_at_index(dependent_index).param->clamp_dependent(value.step(), dependent_value);
    set_plain_at_index(dependent_index, clamped);
  }  
}

}