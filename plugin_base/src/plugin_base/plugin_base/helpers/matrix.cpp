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
  std::string result = tag.short_name.size() ? tag.short_name : tag.name;
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
  std::string result = tag1.short_name.size() ? tag1.short_name : tag1.name;
  if (slots1 > 1) result += " " + std::to_string(slot1 + (tag1.name_one_based ? 1: 0));
  result += " " + (tag2.short_name.size() ? tag2.short_name : tag2.name);
  if (slots2 > 1) result += " " + std::to_string(slot2 + (tag2.name_one_based ? 1 : 0));
  return result;
}

static tab_menu_result
make_copy_failed_result(std::string const& matrix_name)
{
  tab_menu_result result;
  result.show_warning = true;
  result.title = "Copy failed";
  result.content = "No slots available for " + matrix_name + " matrix.";
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

tab_menu_result
tidy_matrix_menu_handler::extra(int module, int slot, int action)
{
  auto const& topo = _state->desc().plugin->modules[module];
  std::vector<std::map<int, plain_value>> route_value_maps;
  int route_count = topo.params[_on_param].info.slot_count;
  for (int r = 0; r < route_count; r++)
    if (_state->get_plain_at(module, slot, _on_param, r).step() != _off_value)
    {
      std::map<int, plain_value> route_value_map;
      for(int p = 0; p < topo.params.size(); p++)
        route_value_map[p] = _state->get_plain_at(module, slot, p, r);
      route_value_maps.push_back(route_value_map);
    }
  _state->clear_module(module, slot);

  // sort
  if (action == 1)
    std::sort(route_value_maps.begin(), route_value_maps.end(), [this, &topo](auto const& l, auto const& r) {
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
      _state->set_plain_at(module, slot, p, r, map.at(p));
  }

  return {};
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

tab_menu_result
cv_routing_menu_handler::move(int module, int source_slot, int target_slot)
{
  // overwrite source_slot with target_slot for source parameter
  _state->move_module_to(module, source_slot, target_slot);
  for(auto const& sources: _matrix_sources)
  {
    auto const& topo = _state->desc().plugin->modules[sources.first];
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if(_state->get_plain_at(sources.first, 0, _on_param, r).step() == _off_value) continue;
      update_matched_slot(sources.first, _source_param, r, module, source_slot, target_slot, sources.second);
    }
  }
  return {};
}

tab_menu_result
cv_routing_menu_handler::swap(int module, int source_slot, int target_slot)
{
  // swap source_slot with target_slot for source parameter
  _state->move_module_to(module, source_slot, target_slot);
  for (auto const& sources : _matrix_sources)
  {
    auto const& topo = _state->desc().plugin->modules[sources.first];
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if (_state->get_plain_at(sources.first, 0, _on_param, r).step() == _off_value) continue;
      if(!update_matched_slot(sources.first, _source_param, r, module, source_slot, target_slot, sources.second))
        update_matched_slot(sources.first, _source_param, r, module, target_slot, source_slot, sources.second);
    }
  }
  return {};
}

tab_menu_result
cv_routing_menu_handler::clear(int module, int slot)
{
  // set any route matching this module to all defaults
  _state->clear_module(module, slot);
  for (auto const& sources : _matrix_sources)
  {
    auto const& topo = _state->desc().plugin->modules[sources.first];
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      int selected_source = _state->get_plain_at(sources.first, 0, _source_param, r).step();
      if ((sources.second[selected_source].module_index == module && sources.second[selected_source].module_slot == slot))
        for (int p = 0; p < topo.params.size(); p++)
          _state->set_plain_at(sources.first, 0, p, r, topo.params[p].domain.default_plain(0, r));
    }
  }
  return {};
}

tab_menu_result
cv_routing_menu_handler::copy(int module, int source_slot, int target_slot)
{
  // copy is a bit annoying since we might run out of slots, so check that first
  std::map<int, int> slots_available;
  std::map<int, std::vector<int>> routes_to_copy;
  for (auto const& sources : _matrix_sources)
  {
    int this_slots_available = 0;
    std::vector<int> this_routes_to_copy;
    auto const& topo = _state->desc().plugin->modules[sources.first];
    for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
    {
      if (_state->get_plain_at(sources.first, 0, _on_param, r).step() == _off_value) this_slots_available++;
      else if(is_selected(sources.first, _source_param, r, module, source_slot, sources.second)) this_routes_to_copy.push_back(r);
    }
    routes_to_copy[sources.first] = this_routes_to_copy;
    slots_available[sources.first] = this_slots_available;
  }

  for (auto const& sources : _matrix_sources)
    if(routes_to_copy.at(sources.first).size() > slots_available.at(sources.first))
    {
      auto const& topo = _state->desc().plugin->modules[sources.first];
      return make_copy_failed_result(topo.info.tag.name);
    }

  // copy each route entirely (all params), only replace source by target
  _state->copy_module_to(module, source_slot, target_slot);
  for (auto const& sources : _matrix_sources)
  {
    auto const& topo = _state->desc().plugin->modules[sources.first];
    for (int rc = 0; rc < routes_to_copy.at(sources.first).size(); rc++)
      for (int r = 0; r < topo.params[_on_param].info.slot_count; r++)
        if (_state->get_plain_at(sources.first, 0, _on_param, r).step() == _off_value)
        {
          for (int p = 0; p < topo.params.size(); p++)
            _state->set_plain_at(sources.first, 0, p, r, _state->get_plain_at(sources.first, 0, p, routes_to_copy.at(sources.first)[rc]));
          update_matched_slot(sources.first, _source_param, r, module, source_slot, target_slot, sources.second);
          break;
        }
   }

  return {};
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

tab_menu_result
audio_routing_menu_handler::move(int module, int source_slot, int target_slot)
{
  // overwrite source_slot with target_slot for both source and target parameter for cv matrix
  _state->move_module_to(module, source_slot, target_slot);
  auto const& cv_topo = _state->desc().plugin->modules[_cv_params.matrix_module];
  for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
    if (_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.on_param, r).step() != _cv_params.off_value)
      update_matched_cv_slot(r, module, source_slot, target_slot);

  // overwrite source_slot with target_slot for both source and target parameter for all audio matrices
  for(int m = 0; m < _audio_params.size(); m++)
  {
    int matrix = _audio_params[m].matrix_module;
    auto const& audio_topo = _state->desc().plugin->modules[matrix];
    for (int r = 0; r < audio_topo.params[_audio_params[m].on_param].info.slot_count; r++)
      if (_state->get_plain_at(matrix, 0, _audio_params[m].on_param, r).step() != _audio_params[m].off_value)
      {
        update_matched_audio_slot(matrix, _audio_params[m].source_param, r, module, source_slot, target_slot, _audio_params[m].sources);
        update_matched_audio_slot(matrix, _audio_params[m].target_param, r, module, source_slot, target_slot, _audio_params[m].targets);
      }
  }
  return {};
}

tab_menu_result
audio_routing_menu_handler::swap(int module, int source_slot, int target_slot)
{
  // swap source_slot with target_slot for both source and target parameter for cv matrix
  _state->move_module_to(module, source_slot, target_slot);
  auto const& cv_topo = _state->desc().plugin->modules[_cv_params.matrix_module];
  for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
    if (_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.on_param, r).step() != _cv_params.off_value)
      if (!update_matched_cv_slot(r, module, source_slot, target_slot))
        update_matched_cv_slot(r, module, target_slot, source_slot);

  // swap source_slot with target_slot for both source and target parameter for all audio matrices
  for(int m = 0; m < _audio_params.size(); m++)
  {
    int matrix = _audio_params[m].matrix_module;
    auto const& audio_topo = _state->desc().plugin->modules[matrix];
    for (int r = 0; r < audio_topo.params[_audio_params[m].on_param].info.slot_count; r++)
      if (_state->get_plain_at(matrix, 0, _audio_params[m].on_param, r).step() != _audio_params[m].off_value)
      {
        if(!update_matched_audio_slot(matrix, _audio_params[m].source_param, r, module, source_slot, target_slot, _audio_params[m].sources))
          update_matched_audio_slot(matrix, _audio_params[m].source_param, r, module, target_slot, source_slot, _audio_params[m].sources);
        if (!update_matched_audio_slot(matrix, _audio_params[m].target_param, r, module, source_slot, target_slot, _audio_params[m].targets))
          update_matched_audio_slot(matrix, _audio_params[m].target_param, r, module, target_slot, source_slot, _audio_params[m].targets);
      }
  }
  return {};
}

tab_menu_result
audio_routing_menu_handler::clear(int module, int slot)
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

  // set any route matching this module to all defaults for all audio matrices
  for(int m = 0; m < _audio_params.size(); m++)
  {
    int matrix = _audio_params[m].matrix_module;
    auto const& audio_topo = _state->desc().plugin->modules[matrix];
    for (int r = 0; r < audio_topo.params[_audio_params[m].on_param].info.slot_count; r++)
    {
      int selected_audio_source = _state->get_plain_at(matrix, 0, _audio_params[m].source_param, r).step();
      int selected_audio_target = _state->get_plain_at(matrix, 0, _audio_params[m].target_param, r).step();
      if ((_audio_params[m].sources[selected_audio_source].index == module && _audio_params[m].sources[selected_audio_source].slot == slot) ||
        (_audio_params[m].targets[selected_audio_target].index == module && _audio_params[m].targets[selected_audio_target].slot == slot))
        for (int p = 0; p < audio_topo.params.size(); p++)
          _state->set_plain_at(matrix, 0, p, r, audio_topo.params[p].domain.default_plain(0, r));
    }
  }
  return {};
}

tab_menu_result
audio_routing_menu_handler::copy(int module, int source_slot, int target_slot)
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

  // check if we have enough slots for all audio matrices
  std::vector<std::vector<int>> audio_routes_to_copy;
  for(int m = 0; m < _audio_params.size(); m++)
  {
    int audio_slots_available = 0;
    audio_routes_to_copy.emplace_back();
    int matrix = _audio_params[m].matrix_module;
    auto const& audio_topo = _state->desc().plugin->modules[matrix];
    for (int r = 0; r < audio_topo.params[_audio_params[m].on_param].info.slot_count; r++)
    {
      if (_state->get_plain_at(matrix, 0, _audio_params[m].on_param, r).step() == _audio_params[m].off_value) audio_slots_available++;
      else if(is_audio_selected(matrix, _audio_params[m].source_param, r, module, source_slot, _audio_params[m].sources)) audio_routes_to_copy[m].push_back(r);
      else if(is_audio_selected(matrix, _audio_params[m].target_param, r, module, source_slot, _audio_params[m].targets)) audio_routes_to_copy[m].push_back(r);
    }

    if (audio_routes_to_copy[m].size() > audio_slots_available)
      return make_copy_failed_result(audio_topo.info.tag.name);
  }

  // copy module
  _state->copy_module_to(module, source_slot, target_slot);

  // update cv routing
  for (int rc = 0; rc < cv_routes_to_copy.size(); rc++)
    for (int r = 0; r < cv_topo.params[_cv_params.on_param].info.slot_count; r++)
      if (_state->get_plain_at(_cv_params.matrix_module, 0, _cv_params.on_param, r).step() == _cv_params.off_value)
      {
        for (int p = 0; p < cv_topo.params.size(); p++)
          _state->set_plain_at(_cv_params.matrix_module, 0, p, r, _state->get_plain_at(_cv_params.matrix_module, 0, p, cv_routes_to_copy[rc]));
        update_matched_cv_slot(r, module, source_slot, target_slot);
        break;
      }

  // update audio routing
  for(int m = 0; m < _audio_params.size(); m++)
  {
    int matrix = _audio_params[m].matrix_module;
    auto const& audio_topo = _state->desc().plugin->modules[matrix];
    for (int rc = 0; rc < audio_routes_to_copy[m].size(); rc++)
      for (int r = 0; r < audio_topo.params[_audio_params[m].on_param].info.slot_count; r++)
        if (_state->get_plain_at(matrix, 0, _audio_params[m].on_param, r).step() == _audio_params[m].off_value)
        {
          for (int p = 0; p < audio_topo.params.size(); p++)
            _state->set_plain_at(matrix, 0, p, r, _state->get_plain_at(matrix, 0, p, audio_routes_to_copy[m][rc]));
          update_matched_audio_slot(matrix, _audio_params[m].source_param, r, module, source_slot, target_slot, _audio_params[m].sources);
          update_matched_audio_slot(matrix, _audio_params[m].target_param, r, module, source_slot, target_slot, _audio_params[m].targets);
          break;
        }
  }

  return {};
}

}
