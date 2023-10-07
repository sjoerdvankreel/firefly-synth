#pragma once

#include <plugin_base/utility.hpp>
#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/module.hpp>

#include <string>

namespace plugin_base {

struct module_desc;

// runtime parameter descriptor
struct param_desc final {
  int topo = {};
  int slot = {};
  int local = {};
  int global = {};
  int id_hash = {};
  std::string id = {};
  std::string name = {};
  std::string full_name = {};
  param_topo const* param = {};

  void validate(module_desc const& module) const;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_desc);

  param_desc(
    module_topo const& module_, int module_slot,
    param_topo const& param_, int topo, int slot, int local, int global);
};

}
