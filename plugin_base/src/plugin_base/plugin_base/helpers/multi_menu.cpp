#include <plugin_base/helpers/multi_menu.hpp>

namespace plugin_base {
      
multi_menu 
make_multi_menu(
  std::vector<topo_tag> const& tags1,
  std::vector<topo_tag> const& tags2,
  std::vector<topo_tag> const& tags3,
  std::function<std::string(int)> make_header,
  std::function<std::string(int, int, int)> make_name)
{
  int index = 0;
  multi_menu result;
  result.submenu = std::make_shared<gui_submenu>();
  for(int i1 = 0; i1 < tags1.size(); i1++)
  {
    auto menu_1 = result.submenu->add_submenu(make_header(i1));
    for (int i2 = 0; i2 < tags2.size(); i2++)
      for (int i3 = 0; i3 < tags3.size(); i3++)
      {
        menu_1->indices.push_back(index++);
        result.multi_items.push_back({ i1, i2, i3 });
        std::string id = tags1[i1].id + "-" + tags2[i2].id + "-" + tags3[i3].id;
        result.items.push_back({ id, make_name(i1, i2, i3)});
      }
  }
  return result;
}

}