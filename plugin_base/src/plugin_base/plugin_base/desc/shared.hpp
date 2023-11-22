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

// only used for midi source descriptors
struct desc_info {
  int topo = {};
  int global = {};
  int id_hash = {};
  std::string id = {};
  std::string name = {};

  void validate(int topo_count) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(desc_info);
};

// param_desc and module_desc info
struct topo_desc_info final:
public desc_info {
  int slot = {};

  void validate(int topo_count, int slot_count) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(topo_desc_info);
};

}
