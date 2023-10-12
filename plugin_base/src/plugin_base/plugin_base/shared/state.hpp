#pragma once

#include <plugin_base/dsp/value.hpp>
#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

namespace plugin_base {

class plugin_state final {
  plugin_desc const _desc;
  jarray<plain_value, 4> _state = {};  

public:
  void init_defaults();
  plugin_state(std::unique_ptr<plugin_topo>&& topo);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_state);  

  plain_value get(int global_index) const;
  void set(int global_index, plain_value value);
  param_desc const& param(int global_index) const;

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& state() { return _state; }
  jarray<plain_value, 4> const& state() const { return _state; }
};

inline param_desc const&
plugin_state::param(int global_index) const
{
  auto const& mapping = desc().mappings.params[global_index];
  return desc().param_at(mapping);
}

inline plain_value 
plugin_state::get(int global_index) const
{
  auto const& mapping = desc().mappings.params[global_index];
  return mapping.value_at(_state);
}

inline void 
plugin_state::set(int global_index, plain_value value)
{
  auto const& mapping = desc().mappings.params[global_index];
  mapping.value_at(_state) = value;
}


}