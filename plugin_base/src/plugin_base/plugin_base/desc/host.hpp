#pragma once

#include <plugin_base/shared/utility.hpp>

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace plugin_base {

// from resources folder
struct factory_preset
{
  std::string name;
  std::string path;
};

enum { 
  host_menu_flags_none = 0x0, 
  host_menu_flags_checked = 0x1,
  host_menu_flags_enabled = 0x2, 
  host_menu_flags_separator = 0x4 };

struct host_menu_item
{
  // no action associated
  static inline int const no_tag = -1;

  int flags;
  int tag = no_tag;
  std::string name;
  std::vector<std::shared_ptr<host_menu_item>> children;
};

// per-param host context menu
struct host_menu
{
  host_menu_item root;
  std::function<void(int tag)> clicked;
};

// differences between plugin formats
struct format_config {
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(format_config);

  virtual ~format_config() {}
  virtual std::unique_ptr<host_menu>
  context_menu(int param_id) const = 0;
  virtual std::filesystem::path 
  resources_folder(std::filesystem::path const& binary_path) const = 0;
};

}