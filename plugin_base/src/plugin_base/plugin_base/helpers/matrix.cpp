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
  std::string result = tag.name;
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
  std::string result = tag1.name;
  if (slots1 > 1) result += " " + std::to_string(slot1 + (tag1.name_one_based ? 1: 0));
  result += " " + tag2.name;
  if (slots2 > 1) result += " " + std::to_string(slot2 + (tag2.name_one_based ? 1 : 0));
  return result;
}

routing_matrix<module_topo_mapping>
make_audio_matrix(std::vector<module_topo const*> const& modules)
{
  int index = 0;
  routing_matrix<module_topo_mapping> result;
  result.submenu = std::make_shared<gui_submenu>();
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
      for (int mi = 0; mi < slots; mi++)
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
make_cv_source_matrix(std::vector<module_topo const*> const& modules)
{
  int index = 0;
  routing_matrix<module_output_mapping> result;
  result.submenu = std::make_shared<gui_submenu>();
  for (int m = 0; m < modules.size(); m++)
  {
    auto const& module_tag = modules[m]->info.tag;
    int module_slots = modules[m]->info.slot_count;
    if (module_slots > 1)
    {
      auto const& output = modules[m]->dsp.outputs[0];
      if(!output.is_modulation_source) continue;
      assert(output.info.slot_count == 1);
      assert(modules[m]->dsp.outputs.size() == 1);
      auto module_submenu = result.submenu->add_submenu(module_tag.name);
      for (int mi = 0; mi < module_slots; mi++)
      {
        module_submenu->indices.push_back(index++);
        result.mappings.push_back({ modules[m]->info.index, mi, 0, 0 });
        result.items.push_back({
          make_id(module_tag.id, mi, output.info.tag.id, 0),
          make_name(module_tag, mi, module_slots) });
      }
    } else if (modules[m]->dsp.outputs.size() == 1 && modules[m]->dsp.outputs[0].info.slot_count == 1)
    {
      auto const& output = modules[m]->dsp.outputs[0];
      if (!output.is_modulation_source) continue;
      result.submenu->indices.push_back(index++);
      result.mappings.push_back({ modules[m]->info.index, 0, 0, 0 });
      result.items.push_back({
        make_id(module_tag.id, 0, output.info.tag.id, 0),
        make_name(module_tag, 0, 1) });
    }
    else
    {
      auto output_submenu = result.submenu->add_submenu(module_tag.name);
      for (int o = 0; o < modules[m]->dsp.outputs.size(); o++)
      {
        auto const& output = modules[m]->dsp.outputs[o];
        if (!output.is_modulation_source) continue;
        for (int oi = 0; oi < output.info.slot_count; oi++)
        {
          output_submenu->indices.push_back(index++);
          result.mappings.push_back({ modules[m]->info.index, 0, o, oi });
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
make_cv_target_matrix(std::vector<plugin_base::module_topo const*> const& modules)
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

tab_menu_result
tidy_matrix_menu_handler::extra(plugin_state* state, int module, int slot, int action)
{
  auto const& topo = state->desc().plugin->modules[module];
  std::vector<std::map<int, plain_value>> route_value_maps;
  int route_count = topo.params[_on_param].info.slot_count;
  for (int r = 0; r < route_count; r++)
    if (state->get_plain_at(module, slot, _on_param, r).step() != _off_value)
    {
      std::map<int, plain_value> route_value_map;
      for(int p = 0; p < topo.params.size(); p++)
        route_value_map[p] = state->get_plain_at(module, slot, p, r);
      route_value_maps.push_back(route_value_map);
    }
  state->clear_module(module, slot);

  // sort
  if (action == 1)
    std::sort(route_value_maps.begin(), route_value_maps.end(), [this, state, &topo](auto const& l, auto const& r) {
      for (int p = 0; p < _sort_params.size(); p++)
      {
        assert(!topo.params[_sort_params[p]].domain.is_real());
        if(l.at(_sort_params[p]).step() < r.at(_sort_params[p]).step())
          return true;
        if (l.at(_sort_params[p]).step() > r.at(_sort_params[p]).step())
          return false;
      }
      return false;
    });

  // tidy
  for (int r = 0; r < route_value_maps.size(); r++)
  {
    auto const& map = route_value_maps[r];
    for(int p = 0; p < topo.params.size(); p++)
      state->set_plain_at(module, slot, p, r, map.at(p));
  }

  return {};
}

bool
cv_routing_menu_handler::is_selected(
  plugin_state* state, int matrix, int param, int route, int module, 
  int slot, std::vector<module_output_mapping> const& mappings)
{
  int selected = state->get_plain_at(matrix, 0, param, route).step();
  return mappings[selected].module_index == module && mappings[selected].module_slot == slot;
}

bool
cv_routing_menu_handler::update_matched_slot(
  plugin_state* state, int matrix, int param, int route, int module,
  int from_slot, int to_slot, std::vector<module_output_mapping> const& mappings)
{
  if (!is_selected(state, matrix, param, route, module, from_slot, mappings)) return false;
  auto replace_iter = std::find_if(mappings.begin(), mappings.end(),
    [module, to_slot](auto const& m) { return m.module_index == module && m.module_slot == to_slot; });
  state->set_raw_at(matrix, 0, param, route, (int)(replace_iter - mappings.begin()));
  return true;
}

tab_menu_result
cv_routing_menu_handler::move(plugin_state* state, int module, int source_slot, int target_slot)
{
  state->move_module_to(module, source_slot, target_slot);

  // overwrite source_slot with target_slot for source parameter
  for(int matrix: _matrix_modules)
  {
    auto const& topo = state->desc().plugin->modules[matrix];
    auto const& sources = make_cv_source_matrix(_sources_factory(state->desc().plugin, matrix)).mappings;
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if(state->get_plain_at(matrix, 0, _on_param, r).step() == _off_value) continue;
      update_matched_slot(state, matrix, _source_param, r, module, source_slot, target_slot, sources);
    }
  }

  return {};
}

tab_menu_result
cv_routing_menu_handler::swap(plugin_state* state, int module, int source_slot, int target_slot)
{
  state->move_module_to(module, source_slot, target_slot);

  // swap source_slot with target_slot for source parameter
  for (int matrix : _matrix_modules)
  {
    auto const& topo = state->desc().plugin->modules[matrix];
    auto const& sources = make_cv_source_matrix(_sources_factory(state->desc().plugin, matrix)).mappings;
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if (state->get_plain_at(matrix, 0, _on_param, r).step() == _off_value) continue;
      if(!update_matched_slot(state, matrix, _source_param, r, module, source_slot, target_slot, sources))
        update_matched_slot(state, matrix, _source_param, r, module, target_slot, source_slot, sources);
    }
  }

  return {};
}

tab_menu_result
cv_routing_menu_handler::clear(plugin_state* state, int module, int slot)
{
  state->clear_module(module, slot);

  // set any route matching this module to all defaults
  for (int matrix : _matrix_modules)
  {
    auto const& topo = state->desc().plugin->modules[matrix];
    auto sources = make_cv_source_matrix(_sources_factory(state->desc().plugin, matrix)).mappings;
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      int selected_source = state->get_plain_at(matrix, 0, _source_param, r).step();
      if ((sources[selected_source].module_index == module && sources[selected_source].module_slot == slot))
        for (int p = 0; p < topo.params.size(); p++)
          state->set_plain_at(matrix, 0, p, r, topo.params[p].domain.default_plain(0, r));
    }
  }

  return {};
}

tab_menu_result
cv_routing_menu_handler::copy(plugin_state* state, int module, int source_slot, int target_slot)
{
  // copy is a bit annoying since we might run out of slots, so check that first
  std::map<int, int> slots_available;
  std::map<int, std::vector<int>> routes_to_copy;
  for (int matrix : _matrix_modules)
  {
    int this_slots_available = 0;
    std::vector<int> this_routes_to_copy;
    auto const& topo = state->desc().plugin->modules[matrix];
    auto const& sources = make_cv_source_matrix(_sources_factory(state->desc().plugin, matrix)).mappings;
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if (state->get_plain_at(matrix, 0, _on_param, r).step() == _off_value) this_slots_available++;
      else if(is_selected(state, matrix, _source_param, r, module, source_slot, sources)) this_routes_to_copy.push_back(r);
    }
    routes_to_copy[matrix] = this_routes_to_copy;
    slots_available[matrix] = this_slots_available;
  }

  for(int m = 0; m < _matrix_modules.size(); m++)
    if(routes_to_copy.at(_matrix_modules[m]).size() > slots_available.at(_matrix_modules[m]))
    {
      tab_menu_result result;
      result.show_warning = true;
      result.title = "Copy failed";
      result.content = "No matrix slots available.";
      return result;
    }

  // copy each route entirely (all params), only replace source by target
  state->copy_module_to(module, source_slot, target_slot);
  for(int matrix: _matrix_modules)
  {
    auto const& topo = state->desc().plugin->modules[matrix];
    auto const& sources = make_cv_source_matrix(_sources_factory(state->desc().plugin, matrix)).mappings;
    for (int rc = 0; rc < routes_to_copy.at(matrix).size(); rc++)
      for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
        if (state->get_plain_at(matrix, 0, _on_param, r).step() == _off_value)
        {
          for (int p = 0; p < topo.params.size(); p++)
            state->set_plain_at(matrix, 0, p, r, state->get_plain_at(matrix, 0, p, routes_to_copy.at(matrix)[rc]));
          update_matched_slot(state, matrix, _source_param, r, module, source_slot, target_slot, sources);
          break;
        }
   }

  return {};
}

bool 
audio_routing_menu_handler::is_selected(
  plugin_state* state, int param, int route, int module, int slot,
  std::vector<module_topo_mapping> const& mappings)
{
  int selected = state->get_plain_at(_matrix_module, 0, param, route).step();
  return mappings[selected].index == module && mappings[selected].slot == slot;
}

bool
audio_routing_menu_handler::update_matched_slot(
  plugin_state* state, int param, int route, int module,
  int from_slot, int to_slot, std::vector<module_topo_mapping> const& mappings)
{
  if(!is_selected(state, param, route, module, from_slot, mappings)) return false;
  auto replace_iter = std::find_if(mappings.begin(), mappings.end(),
    [module, to_slot](auto const& m) { return m.index == module && m.slot == to_slot; });
  state->set_raw_at(_matrix_module, 0, param, route, (int)(replace_iter - mappings.begin()));
  return true;
}

tab_menu_result
audio_routing_menu_handler::move(plugin_state* state, int module, int source_slot, int target_slot)
{
  state->move_module_to(module, source_slot, target_slot);

  // overwrite source_slot with target_slot for both source and target parameter
  auto const& topo = state->desc().plugin->modules[_matrix_module];
  auto const& sources = make_audio_matrix(_sources_factory(state->desc().plugin)).mappings;
  auto const& targets = make_audio_matrix(_targets_factory(state->desc().plugin)).mappings;
  for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
  {
    if(state->get_plain_at(_matrix_module, 0, _on_param, r).step() == _off_value) continue;
    update_matched_slot(state, _source_param, r, module, source_slot, target_slot, sources);
    update_matched_slot(state, _target_param, r, module, source_slot, target_slot, targets);
  }

  return {};
}

tab_menu_result
audio_routing_menu_handler::swap(plugin_state* state, int module, int source_slot, int target_slot)
{
  state->move_module_to(module, source_slot, target_slot);

  // swap source_slot with target_slot for both source and target parameter
  auto const& topo = state->desc().plugin->modules[_matrix_module];
  auto const& sources = make_audio_matrix(_sources_factory(state->desc().plugin)).mappings;
  auto const& targets = make_audio_matrix(_targets_factory(state->desc().plugin)).mappings;
  for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
  {
    if (state->get_plain_at(_matrix_module, 0, _on_param, r).step() == _off_value) continue;
    if(!update_matched_slot(state, _source_param, r, module, source_slot, target_slot, sources))
      update_matched_slot(state, _source_param, r, module, target_slot, source_slot, sources);
    if (!update_matched_slot(state, _target_param, r, module, source_slot, target_slot, targets))
      update_matched_slot(state, _target_param, r, module, target_slot, source_slot, targets);
  }

  return {};
}

tab_menu_result
audio_routing_menu_handler::clear(plugin_state* state, int module, int slot)
{
  state->clear_module(module, slot);

  // set any route matching this module to all defaults
  auto const& topo = state->desc().plugin->modules[_matrix_module];
  auto sources = make_audio_matrix(_sources_factory(state->desc().plugin)).mappings;
  auto targets = make_audio_matrix(_targets_factory(state->desc().plugin)).mappings;
  for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
  {
    int selected_source = state->get_plain_at(_matrix_module, 0, _source_param, r).step();
    int selected_target = state->get_plain_at(_matrix_module, 0, _target_param, r).step();
    if ((sources[selected_source].index == module && sources[selected_source].slot == slot) ||
      (targets[selected_target].index == module && targets[selected_target].slot == slot))
      for (int p = 0; p < topo.params.size(); p++)
        state->set_plain_at(_matrix_module, 0, p, r, topo.params[p].domain.default_plain(0, r));
  }

  return {};
}

tab_menu_result
audio_routing_menu_handler::copy(plugin_state* state, int module, int source_slot, int target_slot)
{
  // copy is a bit annoying since we might run out of slots, so check that first
  int slots_available = 0;
  std::vector<int> routes_to_copy;
  auto const& topo = state->desc().plugin->modules[_matrix_module];
  auto const& sources = make_audio_matrix(_sources_factory(state->desc().plugin)).mappings;
  auto const& targets = make_audio_matrix(_targets_factory(state->desc().plugin)).mappings;
  for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
  {
    if (state->get_plain_at(_matrix_module, 0, _on_param, r).step() == _off_value) slots_available++;
    else if(is_selected(state, _source_param, r, module, source_slot, sources)) routes_to_copy.push_back(r);
    else if(is_selected(state, _target_param, r, module, source_slot, targets)) routes_to_copy.push_back(r);
  }

  if(routes_to_copy.size() > slots_available)
  {
    tab_menu_result result;
    result.show_warning = true;
    result.title = "Copy failed";
    result.content = "No matrix slots available.";
    return result;
  }

  // copy each route entirely (all params), only replace source by target
  state->copy_module_to(module, source_slot, target_slot);
  for (int rc = 0; rc < routes_to_copy.size(); rc++)
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
      if (state->get_plain_at(_matrix_module, 0, _on_param, r).step() == _off_value)
      {
        for (int p = 0; p < topo.params.size(); p++)
          state->set_plain_at(_matrix_module, 0, p, r, state->get_plain_at(_matrix_module, 0, p, routes_to_copy[rc]));
        update_matched_slot(state, _source_param, r, module, source_slot, target_slot, sources);
        update_matched_slot(state, _target_param, r, module, source_slot, target_slot, targets);
        break;
      }

  return {};
}

}
