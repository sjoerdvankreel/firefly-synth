#include <plugin_base/shared/state.hpp>
#include <plugin_base/helpers/matrix.hpp>

namespace plugin_base {

static std::string
make_id(std::string const& id, int slot)
{
  std::string result = id;
  result += "-" + std::to_string(slot);
  return result;
}

static std::string
make_name(topo_tag const& tag, int slot, int slots)
{
  std::string result = (tag.short_name_in_menu && tag.short_name.size()) ? tag.short_name : tag.name;
  if(slots > 1) result += " " + std::to_string(slot + (tag.name_one_based? 1: 0));
  return result;
}

static std::string
make_id(std::string const& id1, int slot1, std::string const& id2, int slot2)
{
  std::string result = id1;
  result += "-" + std::to_string(slot1);
  result += "-" + id2;
  result += "-" + std::to_string(slot2);
  return result;
}

static std::string
make_name(topo_tag const& tag1, int slot1, int slots1, topo_tag const& tag2, int slot2, int slots2)
{
  std::string result = (tag1.short_name_in_menu && tag1.short_name.size()) ? tag1.short_name : tag1.name;
  if (slots1 > 1) result += " " + std::to_string(slot1 + (tag1.name_one_based ? 1: 0));
  result += " " + ((tag2.short_name_in_menu && tag2.short_name.size()) ? tag2.short_name : tag2.name);
  if (slots2 > 1) result += " " + std::to_string(slot2 + (tag2.name_one_based ? 1 : 0));
  return result;
}

static module_tab_menu_result
make_copy_failed_result(std::string const& matrix_name)
{ return module_tab_menu_result("", true, "Copy failed", "No slots available for " + matrix_name + " matrix."); }

std::vector<custom_menu> const 
matrix_param_menu_handler::menus() const
{ 
  custom_menu result;
  result.menu_id = 0;
  result.name = "Route";
  result.entries = { 
    { clear, "Clear" }, 
    { delete_, "Delete" },
    { duplicate, "Duplicate" }, 
    { insert_before, "Insert Before" }, 
    { insert_after, "Insert After" } };
  return { result }; 
}

std::string
matrix_param_menu_handler::execute(
  int menu_id, int action, int module_index,
  int module_slot, int param_index, int param_slot)
{
  auto const& topo = _state->desc().plugin->modules[module_index];
  int section_index = param_index / (topo.params.size() / _section_count);
  int section_param_index = param_index % (topo.params.size() / _section_count);
  execute(menu_id, action, module_index, module_slot, section_index, section_param_index, param_slot);
  return topo.info.tag.name + " Route " + std::to_string(param_slot + 1);
}

void 
matrix_param_menu_handler::execute(
  int menu_id, int action, int module_index, 
  int module_slot, int section_index, int section_param_index, int param_slot)
{
  assert(menu_id == 0);
  assert(0 <= action && action < action_count);

  auto const& topo = _state->desc().plugin->modules[module_index];
  int section_param_count = topo.params.size() / _section_count;

  if (action == clear)
  {
    for (int p = section_index * section_param_count; p < (section_index + 1) * section_param_count; p++)
      _state->set_plain_at(module_index, module_slot, p, param_slot, 
        topo.params[p].domain.default_plain(module_slot, param_slot));
    return;
  }

  if (action == delete_)
  {
    execute(menu_id, clear, module_index, module_slot, section_index, section_param_index, param_slot);
    for(int r = param_slot; r < _route_count - 1; r++)
      for (int p = section_index * section_param_count; p < (section_index + 1) * section_param_count; p++)
        _state->set_plain_at(module_index, module_slot, p, r,
          _state->get_plain_at(module_index, module_slot, p, r + 1));
    execute(menu_id, clear, module_index, module_slot, section_index, section_param_index, _route_count - 1);
    return;
  }
  if (action == insert_before)
    execute(menu_id, clear, module_index, module_slot, section_index, section_param_index, _route_count - 1);
  for (int r = _route_count - 1; r > (action == insert_after? param_slot + 1: param_slot); r--)
    for (int p = section_index * section_param_count; p < (section_index + 1) * section_param_count; p++)
      _state->set_plain_at(module_index, module_slot, p, r,
        _state->get_plain_at(module_index, module_slot, p, r - 1));
  if (action == insert_before)
    execute(menu_id, clear, module_index, module_slot, section_index, section_param_index, param_slot);
  if ((action == insert_after || action == duplicate) && param_slot < _route_count - 1)
    execute(menu_id, clear, module_index, module_slot, section_index, section_param_index, param_slot + 1);
  if(action == insert_before)
    _state->set_raw_at(module_index, module_slot, (section_index * section_param_count) + section_param_index, param_slot, _default_on_value);
  if (action == insert_after && param_slot < _route_count - 1)
    _state->set_raw_at(module_index, module_slot, (section_index * section_param_count) + section_param_index, param_slot + 1, _default_on_value);
  if (action == duplicate && param_slot < _route_count - 1)
    for (int p = section_index * section_param_count; p < (section_index + 1) * section_param_count; p++)
      _state->set_plain_at(module_index, module_slot, p, param_slot + 1,
        _state->get_plain_at(module_index, module_slot, p, param_slot));
  return;
}

routing_matrix<module_topo_mapping>
make_audio_matrix(std::vector<module_topo const*> const& modules, int start_slot)
{
  int index = 0;
  routing_matrix<module_topo_mapping> result;
  result.submenu = std::make_shared<gui_submenu>();
  assert(start_slot == 0 || modules.size() == 1);
  for (int m = 0; m < modules.size(); m++)
  {
    auto const& tag = modules[m]->info.tag;
    int slots = modules[m]->info.slot_count;
    if (slots == 1)
    {
      result.submenu->indices.push_back(index++);
      result.mappings.push_back({ modules[m]->info.index, 0 });
      result.items.push_back({ make_id(tag.id, 0), make_name(tag, 0, slots) });
    } else
    {
      auto module_submenu = result.submenu->add_submenu(tag.name);
      for (int mi = start_slot; mi < slots; mi++)
      {
        module_submenu->indices.push_back(index++);
        result.mappings.push_back({ modules[m]->info.index, mi });
        result.items.push_back({ make_id(tag.id, mi), make_name(tag, mi, slots) });
      }
    }
  }
  return result;
}

routing_matrix<module_output_mapping>
make_cv_source_matrix(std::vector<cv_source_entry> const& entries)
{
  int index = 0;
  routing_matrix<module_output_mapping> result;
  result.submenu = std::make_shared<gui_submenu>();
  for (int e = 0; e < entries.size(); e++)
  {
    assert((entries[e].module == nullptr) != (entries[e].subheader.empty()));
    if (entries[e].module == nullptr)
    {
      result.submenu->add_subheader(entries[e].subheader);
      continue;
    }

    module_topo const* module = entries[e].module;
    auto const& module_tag = module->info.tag;
    int module_slots = module->info.slot_count;
    if (module_slots > 1)
    {
      auto const& output = module->dsp.outputs[0];
      if(!output.is_modulation_source) continue;
      assert(output.info.slot_count == 1);
      assert(module->dsp.outputs.size() == 1);
      auto module_submenu = result.submenu->add_submenu(module_tag.name);
      for (int mi = 0; mi < module_slots; mi++)
      {
        module_submenu->indices.push_back(index++);
        result.mappings.push_back({ module->info.index, mi, 0, 0 });
        result.items.push_back({
          make_id(module_tag.id, mi, output.info.tag.id, 0),
          make_name(module_tag, mi, module_slots) });
      }
    } else if (module->dsp.outputs.size() == 1 && module->dsp.outputs[0].info.slot_count == 1)
    {
      auto const& output = module->dsp.outputs[0];
      if (!output.is_modulation_source) continue;
      result.submenu->indices.push_back(index++);
      result.mappings.push_back({ module->info.index, 0, 0, 0 });
      result.items.push_back({
        make_id(module_tag.id, 0, output.info.tag.id, 0),
        make_name(module_tag, 0, 1) });
    }
    else
    {
      auto output_submenu = result.submenu->add_submenu(module_tag.name);
      for (int o = 0; o < module->dsp.outputs.size(); o++)
      {
        auto const& output = module->dsp.outputs[o];
        if (!output.is_modulation_source) continue;
        for (int oi = 0; oi < output.info.slot_count; oi++)
        {
          output_submenu->indices.push_back(index++);
          result.mappings.push_back({ module->info.index, 0, o, oi });
          result.items.push_back({
            make_id(module_tag.id, 0, output.info.tag.id, oi),
            make_name(module_tag, 0, 1, output.info.tag, oi, output.info.slot_count) });
        }
      }
    }
  }
  return result;
}

routing_matrix<param_topo_mapping>
make_cv_target_matrix(std::vector<module_topo const*> const& modules)
{
  int index = 0;
  routing_matrix<param_topo_mapping> result;
  result.submenu = std::make_shared<gui_submenu>();
  for (int m = 0; m < modules.size(); m++)
  {
    auto const& module_info = modules[m]->info;
    auto module_submenu = result.submenu->add_submenu(module_info.tag.name);
    if (module_info.slot_count == 1)
    {
      for (int p = 0; p < modules[m]->params.size(); p++)
        if (modules[m]->params[p].dsp.can_modulate(0))
        {
          auto const& param_info = modules[m]->params[p].info;
          for (int pi = 0; pi < param_info.slot_count; pi++)
          {
            list_item item = {
              make_id(module_info.tag.id, 0, param_info.tag.id, pi),
              make_name(module_info.tag, 0, 1, param_info.tag, pi, param_info.slot_count) };
            item.param_topo = { modules[m]->info.index, 0, modules[m]->params[p].info.index, pi };
            result.items.push_back(item);
            module_submenu->indices.push_back(index++);
            result.mappings.push_back(item.param_topo);
          }
        }
    }
    else
    {
      for (int mi = 0; mi < module_info.slot_count; mi++)
      {
        auto module_slot_submenu = module_submenu->add_submenu(
          make_name(module_info.tag, mi, module_info.slot_count));
        for (int p = 0; p < modules[m]->params.size(); p++)
          if (modules[m]->params[p].dsp.can_modulate(mi))
          {
            auto const& param_info = modules[m]->params[p].info;
            for (int pi = 0; pi < param_info.slot_count; pi++)
            {
              list_item item = {
                make_id(module_info.tag.id, mi, param_info.tag.id, pi),
                make_name(module_info.tag, mi, module_info.slot_count, param_info.tag, pi, param_info.slot_count) };
              item.param_topo = { module_info.index, mi, param_info.index, pi };
              result.items.push_back(item);
              result.mappings.push_back(item.param_topo);
              module_slot_submenu->indices.push_back(index++);
            }
          }
      }
    }
  }
  return result;
}

std::vector<custom_menu> const
tidy_matrix_menu_handler::custom_menus() const
{
  custom_menu result;
  result.name = "";
  result.menu_id = 0;
  result.entries = { { 0, "Tidy"}, { 1, "Clear"} };
  return { result };
}

module_tab_menu_result
tidy_matrix_menu_handler::execute_custom(int menu_id, int action, int module, int slot)
{
  assert(menu_id == 0);
  assert(action == 0 || action == 1);

  auto const& topo = _state->desc().plugin->modules[module]; 

  // clear
  if(action == 1)
  {
    _state->clear_module(module, slot);
    return module_tab_menu_result(topo.info.tag.name, false, "", "");
  }

  // keep track of current values
  int route_count = topo.params[_on_param].info.slot_count;
  int section_param_count = topo.params.size() / _section_count;
  std::vector<std::vector<std::map<int, plain_value>>> route_value_maps;

  for(int s = 0; s < _section_count; s++)
  {
    route_value_maps.emplace_back();
    for (int r = 0; r < route_count; r++)
      if (_state->get_plain_at(module, slot, _on_param + s * section_param_count, r).step() != _off_value)
      {
        std::map<int, plain_value> route_value_map;
        for(int p = s * section_param_count; p < (s + 1) * section_param_count; p++)
          route_value_map[p] = _state->get_plain_at(module, slot, p, r);
        route_value_maps[s].push_back(route_value_map);
      }
  }
  _state->clear_module(module, slot);

  // sort
  for(int s = 0; s < _section_count; s++)
    std::sort(route_value_maps[s].begin(), route_value_maps[s].end(), [this, &topo, s](auto const& l, auto const& r) {
      for (int p = 0; p < _sort_params.size(); p++)
      {
        assert(!topo.params[_sort_params[s][p]].domain.is_real());
        if(l.at(_sort_params[s][p]).step() < r.at(_sort_params[s][p]).step()) return true;
        if (l.at(_sort_params[s][p]).step() > r.at(_sort_params[s][p]).step()) return false;
      }
      return false;
    });

  // tidy
  for(int s = 0; s < _section_count; s++)
    for (int r = 0; r < route_value_maps[s].size(); r++)
    {
      auto const& map = route_value_maps[s][r];
      for (int p = s * section_param_count; p < (s + 1) * section_param_count; p++)
        _state->set_plain_at(module, slot, p, r, map.at(p));
    }

  return module_tab_menu_result(topo.info.tag.name, false, "", "");
}

bool
cv_routing_menu_handler::is_selected(
  int matrix, int param, int route, int module, 
  int slot, std::vector<module_output_mapping> const& mappings)
{
  int selected = _state->get_plain_at(matrix, 0, param, route).step();
  return mappings[selected].module_index == module && mappings[selected].module_slot == slot;
}

bool
cv_routing_menu_handler::update_matched_slot(
  int matrix, int param, int route, int module, int from_slot, 
  int to_slot, std::vector<module_output_mapping> const& mappings)
{
  if (!is_selected(matrix, param, route, module, from_slot, mappings)) return false;
  auto replace_iter = std::find_if(mappings.begin(), mappings.end(),
    [module, to_slot](auto const& m) { return m.module_index == module && m.module_slot == to_slot; });
  _state->set_raw_at(matrix, 0, param, route, (int)(replace_iter - mappings.begin()));
  return true;
}

std::vector<module_tab_menu_handler::module_menu>
cv_routing_menu_handler::module_menus() const
{
  module_menu plain_menu;
  plain_menu.name = "";
  plain_menu.menu_id = menu_plain;
  plain_menu.actions = {
    module_tab_menu_handler::clear, module_tab_menu_handler::copy_to, module_tab_menu_handler::swap_with };
  module_menu routing_menu;
  routing_menu.name = "With CV Routing";
  routing_menu.menu_id = menu_with_cv;
  routing_menu.actions = {
    module_tab_menu_handler::clear, module_tab_menu_handler::clear_all,
    module_tab_menu_handler::delete_, module_tab_menu_handler::insert_before,
    module_tab_menu_handler::insert_after, module_tab_menu_handler::move_to, module_tab_menu_handler::swap_with };
  return { plain_menu, routing_menu };
}

module_tab_menu_result
cv_routing_menu_handler::execute_module(int menu_id, int action, int module, int source_slot, int target_slot)
{
  assert(menu_id == menu_plain || menu_id == menu_with_cv);

  int slot_count = _state->desc().plugin->modules[module].info.slot_count;
  std::string base_item = _state->desc().plugin->modules[module].info.tag.name;
  std::string source_item = base_item;
  std::string target_item = base_item;
  if(slot_count > 1) source_item += " " + std::to_string(source_slot + 1);
  if (slot_count > 1) target_item += " " + std::to_string(target_slot + 1);

  assert(base_item.size());

  if(menu_id == menu_plain)
  {
    switch (action)
    {
    case module_tab_menu_handler::clear: _state->clear_module(module, source_slot); break;
    case module_tab_menu_handler::copy_to: _state->copy_module_to(module, source_slot, target_slot); break;
    case module_tab_menu_handler::swap_with: _state->swap_module_with(module, source_slot, target_slot); break;
    default: assert(false); break;
    }
    return module_tab_menu_result(target_item, false, "", "");
  }

  if(menu_id == menu_with_cv)
  {
    switch (action)
    {
    case module_tab_menu_handler::clear_all: 
      clear_all(module); 
      return module_tab_menu_result(base_item, false, "", "");
    case module_tab_menu_handler::move_to: 
      move_to(module, source_slot, target_slot);
      return module_tab_menu_result(target_item, false, "", "");
    case module_tab_menu_handler::swap_with: 
      swap_with(module, source_slot, target_slot);
      return module_tab_menu_result(target_item, false, "", "");
    case module_tab_menu_handler::clear: clear(module, source_slot); break;
    case module_tab_menu_handler::delete_: delete_(module, source_slot); break;
    case module_tab_menu_handler::insert_after: insert_after(module, source_slot); break;
    case module_tab_menu_handler::insert_before: insert_before(module, source_slot); break;
    default: assert(false); break;
    }
    return module_tab_menu_result(source_item, false, "", "");
  }
  
  assert(false);
  return module_tab_menu_result("", false, "", "");
}

void 
cv_routing_menu_handler::clear_all(int module)
{
  // set any route matching any module to all defaults
  auto const& topo = _state->desc().plugin->modules[module];
  for (int i = 0; i < topo.info.slot_count; i++)
    clear(module, i);
}

void
cv_routing_menu_handler::delete_(int module, int slot)
{
  // move all from slot to the left
  auto const& topo = _state->desc().plugin->modules[module];
  clear(module, slot);
  for (int i = slot; i < topo.info.slot_count - 1; i++)
    move_to(module, i + 1, i);
  clear(module, topo.info.slot_count - 1);
}

void
cv_routing_menu_handler::insert(int module, int slot, bool after)
{
  // move all from slot to the right
  auto const& topo = _state->desc().plugin->modules[module];
  clear(module, topo.info.slot_count - 1);
  for (int i = topo.info.slot_count - 1; i > (after? slot + 1: slot); i--)
    move_to(module, i - 1, i);
}

void 
cv_routing_menu_handler::move_to(int module, int source_slot, int target_slot)
{
  // overwrite source_slot with target_slot for source parameter
  clear(module, target_slot);
  _state->move_module_to(module, source_slot, target_slot);
  for (auto const& matrix : _source_matrices)
  {
    auto const& topo = _state->desc().plugin->modules[matrix.first];
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if (_state->get_plain_at(matrix.first, 0, _on_param, r).step() == _off_value) continue;
      update_matched_slot(matrix.first, _source_param, r, module, source_slot, target_slot, matrix.second);
    }
  }
}

void 
cv_routing_menu_handler::swap_with(int module, int source_slot, int target_slot)
{
  // swap source_slot with target_slot for source parameter
  _state->swap_module_with(module, source_slot, target_slot);
  for (auto const& matrix : _source_matrices)
  {
    auto const& topo = _state->desc().plugin->modules[matrix.first];
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if (_state->get_plain_at(matrix.first, 0, _on_param, r).step() == _off_value) continue;
      if (!update_matched_slot(matrix.first, _source_param, r, module, source_slot, target_slot, matrix.second))
        update_matched_slot(matrix.first, _source_param, r, module, target_slot, source_slot, matrix.second);
    }
  }
}

void
cv_routing_menu_handler::clear(int module, int slot)
{
  // set any route matching this module to all defaults
  _state->clear_module(module, slot);
  for (auto const& matrix : _source_matrices)
  {
    auto const& topo = _state->desc().plugin->modules[matrix.first];
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      int selected_source = _state->get_plain_at(matrix.first, 0, _source_param, r).step();
      if ((matrix.second[selected_source].module_index == module && matrix.second[selected_source].module_slot == slot))
        for (int p = 0; p < topo.params.size(); p++)
          _state->set_plain_at(matrix.first, 0, p, r, topo.params[p].domain.default_plain(0, r));
    }
  }
}

bool 
audio_routing_menu_handler::is_audio_selected(
  int matrix, int param, int route, int module, int slot,
  std::vector<module_topo_mapping> const& mappings)
{
  int selected = _state->get_plain_at(matrix, 0, param, route).step();
  return mappings[selected].index == module && mappings[selected].slot == slot;
}

bool
audio_routing_menu_handler::update_matched_audio_slot(
  int matrix, int param, int route, int module, int from_slot,
  int to_slot, std::vector<module_topo_mapping> const& mappings)
{
  if(!is_audio_selected(matrix, param, route, module, from_slot, mappings)) return false;
  auto replace_iter = std::find_if(mappings.begin(), mappings.end(),
    [module, to_slot](auto const& m) { return m.index == module && m.slot == to_slot; });
  _state->set_raw_at(matrix, 0, param, route, (int)(replace_iter - mappings.begin()));
  return true;
}

bool
audio_routing_menu_handler::is_cv_selected(
  int route, int module, int slot,
  std::vector<param_topo_mapping> const& mappings)
{
  int selected = _state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.target_param, route).step();
  return mappings[selected].module_index == module && mappings[selected].module_slot == slot;
}

bool
audio_routing_menu_handler::update_matched_cv_slot(
  int route, int module, int from_slot, int to_slot)
{
  if (!is_cv_selected(route, module, from_slot, _cv_params.targets)) return false;
  auto const& mapping = _cv_params.targets[_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.target_param, route).step()];
  auto replace_iter = std::find_if(_cv_params.targets.begin(), _cv_params.targets.end(), [module, to_slot, mapping](auto const& m) {
    return m.module_index == module && m.module_slot == to_slot && m.param_index == mapping.param_index && m.param_slot == mapping.param_slot; });
  _state->set_raw_at(_cv_params.matrix_module, 0, _cv_params.target_param, route, (int)(replace_iter - _cv_params.targets.begin()));
  return true;
}

std::vector<module_tab_menu_handler::module_menu>
audio_routing_menu_handler::module_menus() const
{
  module_menu plain_menu;
  plain_menu.menu_id = menu_plain;
  plain_menu.name = "";
  plain_menu.actions = { module_tab_menu_handler::clear, module_tab_menu_handler::copy_to };
  module_menu cv_menu;
  cv_menu.menu_id = menu_with_cv;
  cv_menu.name = "With CV Routing";
  cv_menu.actions = { 
    module_tab_menu_handler::copy_to, module_tab_menu_handler::move_to, module_tab_menu_handler::swap_with };
  module_menu all_menu;
  all_menu.menu_id = menu_with_all;
  all_menu.name = "With CV & Audio Routing";
  all_menu.actions = {
    module_tab_menu_handler::clear, module_tab_menu_handler::clear_all,
    module_tab_menu_handler::delete_, module_tab_menu_handler::insert_after,
    module_tab_menu_handler::insert_before };
  return { plain_menu, cv_menu, all_menu };
}

module_tab_menu_result
audio_routing_menu_handler::execute_module(int menu_id, int action, int module, int source_slot, int target_slot)
{
  int slot_count = _state->desc().plugin->modules[module].info.slot_count;
  std::string base_item = _state->desc().plugin->modules[module].info.tag.name;
  std::string source_item = base_item;
  std::string target_item = base_item;
  if (slot_count > 1) source_item += " " + std::to_string(source_slot + 1);
  if (slot_count > 1) target_item += " " + std::to_string(target_slot + 1);

  assert(base_item.size());

  assert(menu_id == menu_plain || menu_id == menu_with_cv || menu_id == menu_with_all);
  if(menu_id == menu_plain)
  {
    if (action == module_tab_menu_handler::clear)
      _state->clear_module(module, source_slot);
    else if(action == module_tab_menu_handler::copy_to)
      _state->copy_module_to(module, source_slot, target_slot);
    else
      assert(false);
    return module_tab_menu_result(target_item, false, "", "");
  }

  if(menu_id == menu_with_cv)
    switch (action)
    {
    case module_tab_menu_handler::copy_to: 
      return with_cv_copy_to(module, source_slot, target_slot);
    case module_tab_menu_handler::move_to: 
      with_cv_move_to(module, source_slot, target_slot); 
      return module_tab_menu_result(target_item, false, "", "");
    case module_tab_menu_handler::swap_with: 
      with_cv_swap_with(module, source_slot, target_slot);
      return module_tab_menu_result(target_item, false, "", "");
    default: assert(false); return module_tab_menu_result("", false, "", "");
    }

  if(menu_id == menu_with_all)
    switch (action)
    {
    case module_tab_menu_handler::clear_all: 
      with_all_clear_all(module);
      return module_tab_menu_result(base_item, false, "", "");
    case module_tab_menu_handler::clear: with_all_clear(module, source_slot); break;
    case module_tab_menu_handler::delete_: with_all_delete(module, source_slot); break;
    case module_tab_menu_handler::insert_after: with_all_insert_after(module, source_slot); break;
    case module_tab_menu_handler::insert_before: with_all_insert_before(module, source_slot); break;
    default: assert(false);
    }

  return module_tab_menu_result(source_item, false, "", "");
}

void 
audio_routing_menu_handler::clear_cv_route(int route)
{
  auto const& cv_topo = _state->desc().plugin->modules[_cv_params.matrix_module];
  for (int p = 0; p < cv_topo.params.size(); p++)
    _state->set_plain_at(_cv_params.matrix_module, 0, p, route, cv_topo.params[p].domain.default_plain(0, route));
}

void 
audio_routing_menu_handler::move_audio_to(int module, int source_slot, int target_slot)
{
  for (int m = 0; m < _audio_params.size(); m++)
  {
    int matrix = _audio_params[m].matrix_module;
    auto const& matrix_topo = _state->desc().plugin->modules[matrix];
    int route_count = matrix_topo.params[_audio_params[m].matrix_on_params[0]].info.slot_count;
    int section_count = _audio_params[m].matrix_section_count;
    for(int s = 0; s < section_count; s++)
      for (int r = 0; r < route_count; r++)
        if (_state->get_plain_at(matrix, 0, _audio_params[m].matrix_on_params[s], r).step() != _audio_params[m].off_value)
        {
          update_matched_audio_slot(matrix, _audio_params[m].matrix_source_params[s], r, module, source_slot, target_slot, _audio_params[m].sources);
          update_matched_audio_slot(matrix, _audio_params[m].matrix_target_params[s], r, module, source_slot, target_slot, _audio_params[m].targets);
        }
  }
}

void
audio_routing_menu_handler::with_all_clear_all(int module)
{
  auto const& topo = _state->desc().plugin->modules[module];
  for (int i = 0; i < topo.info.slot_count; i++)
    with_all_clear(module, i);
}

void
audio_routing_menu_handler::with_all_delete(int module, int slot)
{
  // move all from slot to the left
  auto const& topo = _state->desc().plugin->modules[module];
  with_all_clear(module, slot);
  for (int i = slot; i < topo.info.slot_count - 1; i++)
  {
    with_cv_move_to(module, i + 1, i);
    move_audio_to(module, i + 1, i);
  }
  with_all_clear(module, topo.info.slot_count - 1);
}

void
audio_routing_menu_handler::with_all_insert(int module, int slot, bool after)
{
  // move all from slot to the right
  auto const& topo = _state->desc().plugin->modules[module];
  with_all_clear(module, topo.info.slot_count - 1);
  for (int i = topo.info.slot_count - 1; i > (after? slot + 1: slot); i--)
  {
    with_cv_move_to(module, i - 1, i);
    move_audio_to(module, i - 1, i);
  }
}

void 
audio_routing_menu_handler::with_cv_move_to(int module, int source_slot, int target_slot)
{
  _state->move_module_to(module, source_slot, target_slot);
  auto const& cv_topo = _state->desc().plugin->modules[_cv_params.matrix_module];
  for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
    if (_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.on_param, r).step() != _cv_params.off_value)
      if(is_cv_selected(r, module, target_slot, _cv_params.targets))
        clear_cv_route(r);
      else
        update_matched_cv_slot(r, module, source_slot, target_slot);
}

void 
audio_routing_menu_handler::with_cv_swap_with(int module, int source_slot, int target_slot)
{
  _state->swap_module_with(module, source_slot, target_slot);
  auto const& cv_topo = _state->desc().plugin->modules[_cv_params.matrix_module];
  for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
    if (_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.on_param, r).step() != _cv_params.off_value)
      if (!update_matched_cv_slot(r, module, source_slot, target_slot))
        update_matched_cv_slot(r, module, target_slot, source_slot);
}

module_tab_menu_result
audio_routing_menu_handler::with_cv_copy_to(int module, int source_slot, int target_slot)
{
  // check if we have enough slots for cv matrix
  int cv_slots_available = 0;
  std::vector<int> cv_routes_to_copy;
  auto const& cv_topo = _state->desc().plugin->modules[_cv_params.matrix_module];
  for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
  {
    if (_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.on_param, r).step() == _cv_params.off_value) cv_slots_available++;
    else if (is_cv_selected(r, module, source_slot, _cv_params.targets)) cv_routes_to_copy.push_back(r);
  }

  if (cv_routes_to_copy.size() > cv_slots_available)
    return make_copy_failed_result(cv_topo.info.tag.name);

  // copy module and update cv routing
  with_cv_clear(module, target_slot);
  _state->copy_module_to(module, source_slot, target_slot);
  for (int rc = 0; rc < cv_routes_to_copy.size(); rc++)
    for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
      if (_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.on_param, r).step() == _cv_params.off_value)
      {
        for (int p = 0; p < cv_topo.params.size(); p++)
          _state->set_plain_at(_cv_params.matrix_module, 0, p, r, _state->get_plain_at(_cv_params.matrix_module, 0, p, cv_routes_to_copy[rc]));
        update_matched_cv_slot(r, module, source_slot, target_slot);
        break;
      }

  int slot_count = _state->desc().plugin->modules[module].info.slot_count;
  std::string base_item = _state->desc().plugin->modules[module].info.tag.name;
  std::string target_item = base_item;
  if (slot_count > 1) target_item += " " + std::to_string(target_slot + 1);
  return module_tab_menu_result(target_item, false, "", "");
}

void
audio_routing_menu_handler::with_cv_clear(int module, int slot)
{
  // set any route matching this module to all defaults for cv matrix
  _state->clear_module(module, slot);
  auto const& cv_topo = _state->desc().plugin->modules[_cv_params.matrix_module];
  for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
  {
    int selected_cv_target = _state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.target_param, r).step();
    if (_cv_params.targets[selected_cv_target].module_index == module && _cv_params.targets[selected_cv_target].module_slot == slot)
      for (int p = 0; p < cv_topo.params.size(); p++)
        _state->set_plain_at(_cv_params.matrix_module, 0, p, r, cv_topo.params[p].domain.default_plain(0, r));
  }
}

void 
audio_routing_menu_handler::with_all_clear(int module, int slot)
{ 
  with_cv_clear(module, slot);

  // set any route matching this module to all defaults for all audio matrices
  for (int m = 0; m < _audio_params.size(); m++)
  {
    int matrix = _audio_params[m].matrix_module;
    auto const& matrix_topo = _state->desc().plugin->modules[matrix];
    int section_count = _audio_params[m].matrix_section_count;
    int section_param_count = matrix_topo.params.size() / section_count;
    int route_count = matrix_topo.params[_audio_params[m].matrix_on_params[0]].info.slot_count;
    assert(_audio_params[m].matrix_section_count == _audio_params[m].matrix_on_params.size());
    assert(_audio_params[m].matrix_section_count == _audio_params[m].matrix_source_params.size());
    assert(_audio_params[m].matrix_section_count == _audio_params[m].matrix_target_params.size());
    for(int s = 0; s < section_count; s++)
      for (int r = 0; r < route_count; r++)
      {
        int selected_audio_source = _state->get_plain_at(matrix, 0, _audio_params[m].matrix_source_params[s], r).step();
        int selected_audio_target = _state->get_plain_at(matrix, 0, _audio_params[m].matrix_target_params[s], r).step();
        if ((_audio_params[m].sources[selected_audio_source].index == module && _audio_params[m].sources[selected_audio_source].slot == slot) ||
          (_audio_params[m].targets[selected_audio_target].index == module && _audio_params[m].targets[selected_audio_target].slot == slot))
          for (int p = s * section_param_count; p < (s + 1) * section_param_count; p++)
            _state->set_plain_at(matrix, 0, p, r, matrix_topo.params[p].domain.default_plain(0, r));
      }
  }
}

}
