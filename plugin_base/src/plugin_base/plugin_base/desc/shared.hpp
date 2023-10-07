#pragma once

#include <plugin_base/topo/module.hpp>
#include <string>

namespace plugin_base {

int
desc_id_hash(std::string const& text);
std::string
desc_id(topo_info const& info, int slot);
std::string
desc_name(topo_info const& info, int slot);

// param_desc and module_desc info
struct desc_info final {
  int topo = {};
  int slot = {};
  int global = {};
  int id_hash = {};
  std::string id = {};
  std::string name = {};

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(desc_info);
  void validate(int topo_count, int slot_count) const;
};

}
