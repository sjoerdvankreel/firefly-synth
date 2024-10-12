#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

// https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
static bool 
string_replace(std::string& str, std::string const& from, std::string const& to) {
  std::size_t start_pos = str.find(from);
  if(start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}
    
static void 
string_replace_all(std::string &str, std::string const& from, std::string const& to)
{
  while(string_replace(str, from, to));
}

param_desc::
param_desc(
  module_topo const& module_, int module_slot,
  param_topo const& param_, int topo, int slot, int local_, int global)
{
  local = local_;
  param = &param_;
  info.topo = topo;
  info.slot = slot;
  info.global = global;
  info.name = desc_name(param_.info, param_.info.tag.full_name, slot);
  full_name = desc_name(module_.info, module_.info.tag.menu_display_name, module_slot) + " " + info.name;
  info.id = desc_id(module_.info, module_slot) + "-" + desc_id(param_.info, slot);
  info.id_hash = stable_hash(info.id.c_str());
} 

void
param_desc::validate(module_desc const& module, int index) const
{
  assert(param);
  assert(local == index);
  assert(info.topo == param->info.index);
  assert(info.name.size() < full_name.size());
  info.validate(module.params.size(), param->info.slot_count);
}

std::string 
param_desc::tooltip(plain_value plain) const
{
  std::string value_text;
  if (param->domain.type == domain_type::item)
    value_text = param->domain.plain_to_item_tooltip(plain);
  else
    value_text = param->domain.plain_to_text(false, plain);
  std::string result = info.name + std::string(": ") + value_text;
  if (param->info.description.size())
    result += std::string("\n\n") + param->info.description;
  // descriptions are in html format
  string_replace_all(result, "<br/>", "\n");
  return result;
}

}
