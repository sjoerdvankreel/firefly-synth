#pragma once

#include <plugin_base/topo/plugin.hpp>

#include <vector>
#include <functional>

// helpers for 3d nested menus
namespace plugin_base {

struct multi_menu_item {
  int index1;
  int index2;
  int index3;
};

struct multi_menu {
  std::vector<list_item> items;
  std::vector<multi_menu_item> multi_items;
  std::shared_ptr<gui_submenu> submenu;
};

multi_menu 
make_multi_menu(
  std::vector<topo_tag> const& tags1, 
  std::vector<topo_tag> const& tags2, 
  std::vector<topo_tag> const& tags3,
  std::function<std::string(int)> make_header,
  std::function<std::string(int, int, int)> make_name);

}