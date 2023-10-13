#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/value.hpp>
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

  param_desc const& param_at_tag(int tag) const;
  param_desc const& param_at_index(int index) const;

  double get_raw_at_tag(int tag) const;
  void set_raw_at_tag(int tag, double value);
  double get_raw_at_index(int index) const;
  void set_raw_at_index(int index, double value);

  plain_value get_plain_at_tag(int tag) const;
  void set_plain_at_tag(int tag, plain_value value);
  plain_value get_plain_at_index(int index) const;
  void set_plain_at_index(int index, plain_value value);

  normalized_value get_normalized_at_tag(int tag) const;
  void set_normalized_at_tag(int tag, normalized_value value);
  normalized_value get_normalized_at_index(int index) const;
  void set_normalized_at_index(int index, normalized_value value);

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& state() { return _state; }
  jarray<plain_value, 4> const& state() const { return _state; }
};

inline param_desc const&
plugin_state::param_at_index(int index) const
{
  auto const& mapping = desc().mappings.params[index];
  return desc().param_at(mapping);
}

inline param_desc const&
plugin_state::param_at_tag(int tag) const
{
  int index = desc().mappings.tag_to_index.at(tag);
  return param_at_index(index);
}

inline plain_value 
plugin_state::get_plain_at_index(int index) const
{
  auto const& mapping = desc().mappings.params[index];
  return mapping.value_at(_state);
}

inline void 
plugin_state::set_plain_at_index(int index, plain_value value)
{
  auto const& mapping = desc().mappings.params[index];
  mapping.value_at(_state) = value;
}

inline plain_value
plugin_state::get_plain_at_tag(int tag) const
{
  int index = desc().mappings.tag_to_index.at(tag);
  return get_plain_at_index(index);
}

inline void
plugin_state::set_plain_at_tag(int tag, plain_value value)
{
  int index = desc().mappings.tag_to_index.at(tag);
  set_plain_at_index(index, value);
}

inline double
plugin_state::get_raw_at_index(int index) const
{
  auto const& mapping = desc().mappings.params[index];
  auto const& domain = desc().param_at(mapping).param->domain;
  return domain.plain_to_raw(mapping.value_at(_state));
}

inline void
plugin_state::set_raw_at_index(int index, double value)
{
  auto const& mapping = desc().mappings.params[index];
  auto const& domain = desc().param_at(mapping).param->domain;
  mapping.value_at(_state) = domain.raw_to_plain(value);
}

inline double
plugin_state::get_raw_at_tag(int tag) const
{
  int index = desc().mappings.tag_to_index.at(tag);
  return get_raw_at_index(index);
}

inline void
plugin_state::set_raw_at_tag(int tag, double value)
{
  int index = desc().mappings.tag_to_index.at(tag);
  set_raw_at_index(index, value);
}

inline normalized_value
plugin_state::get_normalized_at_index(int index) const
{
  auto const& mapping = desc().mappings.params[index];
  auto const& domain = desc().param_at(mapping).param->domain;
  return domain.plain_to_normalized(mapping.value_at(_state));
}

inline void
plugin_state::set_normalized_at_index(int index, normalized_value value)
{
  auto const& mapping = desc().mappings.params[index];
  auto const& domain = desc().param_at(mapping).param->domain;
  mapping.value_at(_state) = domain.normalized_to_plain(value);
}

inline normalized_value
plugin_state::get_normalized_at_tag(int tag) const
{
  int index = desc().mappings.tag_to_index.at(tag);
  return get_normalized_at_index(index);
}

inline void
plugin_state::set_normalized_at_tag(int tag, normalized_value value)
{
  int index = desc().mappings.tag_to_index.at(tag);
  set_normalized_at_index(index, value);
}

}