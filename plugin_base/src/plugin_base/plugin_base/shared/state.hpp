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
  plugin_state(std::unique_ptr<plugin_topo>&& topo);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_state);  

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& state() { return _state; }
  jarray<plain_value, 4> const& state() const { return _state; }
};

}