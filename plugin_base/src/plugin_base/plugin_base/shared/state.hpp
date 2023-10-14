#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

namespace plugin_base {

class plugin_state final {
  jarray<plain_value, 4> _state = {};  
  plugin_desc const* const _desc = {};

public:
  void init_defaults();
  plugin_state(plugin_desc const* desc);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_state);

  // TODO drop all of these
  plugin_desc const& desc() const { return *_desc; }
  jarray<plain_value, 4>& state() { return _state; }
  jarray<plain_value, 4> const& state() const { return _state; }

  plain_value get_plain_at_index(int index) const 
  { return desc().mappings.params[index].value_at(_state); }
  void set_plain_at_index(int index, plain_value value) 
  { desc().mappings.params[index].value_at(_state) = value; }
  plain_value get_plain_at_tag(int tag) const 
  { return get_plain_at_index(desc().mappings.tag_to_index.at(tag)); }
  void set_plain_at_tag(int tag, plain_value value) 
  { set_plain_at_index(desc().mappings.tag_to_index.at(tag), value); }

  double get_raw_at_tag(int tag) const 
  { return get_raw_at_index(desc().mappings.tag_to_index.at(tag)); }
  void set_raw_at_tag(int tag, double value) 
  { set_raw_at_index(desc().mappings.tag_to_index.at(tag), value); }
  double get_raw_at_index(int index) const 
  { return desc().plain_to_raw_at_index(index, desc().mappings.params[index].value_at(_state)); }
  void set_raw_at_index(int index, double value) 
  { desc().mappings.params[index].value_at(_state) = desc().raw_to_plain_at_index(index, value); }

  normalized_value get_normalized_at_tag(int tag) const 
  { return get_normalized_at_index(desc().mappings.tag_to_index.at(tag)); }
  void set_normalized_at_tag(int tag, normalized_value value) 
  { set_normalized_at_index(desc().mappings.tag_to_index.at(tag), value); }
  normalized_value get_normalized_at_index(int index) const 
  { return desc().plain_to_normalized_at_index(index, desc().mappings.params[index].value_at(_state)); }
  void set_normalized_at_index(int index, normalized_value value) 
  { desc().mappings.params[index].value_at(_state) = desc().normalized_to_plain_at_index(index, value); }
};

}