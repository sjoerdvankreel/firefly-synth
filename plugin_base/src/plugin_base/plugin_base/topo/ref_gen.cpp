#include <plugin_base/topo/ref_gen.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <sstream>

namespace plugin_base {

static void
print_topo_stats(
  plugin_topo const& topo);
static void
generate_modules_ref(
  plugin_topo const& topo, std::ostream& out);
static void
generate_plugin_ref(
  plugin_topo const& topo, std::ostream& out);
static void
generate_params_ref(
  plugin_topo const& topo, std::ostream& out);

void
plugin_topo_generate_reference(
  plugin_topo const* topo, 
  plugin_topo_on_reference_generated on_generated, 
  void* ctx)
{
  std::ostringstream out;
  generate_plugin_ref(*topo, out);
  print_topo_stats(*topo);
  std::string reference = out.str();
  on_generated(reference.c_str(), ctx);
}

void
print_topo_stats(
  plugin_topo const& topo)
{
  int total_mod_count = 0;
  int total_param_count = 0;
  int total_block_param_count = 0;
  int total_accurate_param_count = 0;
  int total_voice_start_param_count = 0;
  int total_voice_accurate_param_count = 0;

  int unique_mod_count = 0;
  int unique_param_count = 0;
  int unique_block_param_count = 0;
  int unique_accurate_param_count = 0;
  int unique_voice_start_param_count = 0;
  int unique_voice_accurate_param_count = 0;

  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    unique_mod_count++;
    total_mod_count += topo.modules[m].info.slot_count;
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      unique_param_count++;
      total_param_count += module.info.slot_count * param.info.slot_count;
      if(param.dsp.rate == param_rate::block) unique_block_param_count++;
      if(param.dsp.rate == param_rate::block) total_block_param_count += module.info.slot_count * param.info.slot_count;
      if(param.dsp.rate == param_rate::voice) unique_voice_start_param_count++;
      if(param.dsp.rate == param_rate::voice) total_voice_start_param_count += module.info.slot_count * param.info.slot_count;
      if(param.dsp.rate == param_rate::accurate) unique_accurate_param_count++;
      if(param.dsp.rate == param_rate::accurate) total_accurate_param_count += module.info.slot_count * param.info.slot_count;
      if(param.dsp.rate == param_rate::accurate && module.dsp.stage == module_stage::voice) unique_voice_accurate_param_count++;
      if(param.dsp.rate == param_rate::accurate && module.dsp.stage == module_stage::voice) total_voice_accurate_param_count += module.info.slot_count * param.info.slot_count;
    }
  }

  std::cout << "Total modules: " << total_mod_count << ".\n";
  std::cout << "Unique modules: " << unique_mod_count << ".\n";
  std::cout << "Total params: " << total_param_count << ".\n";
  std::cout << "Unique params: " << unique_param_count << ".\n";
  std::cout << "Total block params: " << total_block_param_count << ".\n";
  std::cout << "Unique block params: " << unique_block_param_count << ".\n";
  std::cout << "Total voice-start params: " << total_voice_start_param_count << ".\n";
  std::cout << "Unique voice-start params: " << unique_voice_start_param_count << ".\n";
  std::cout << "Total accurate params: " << total_accurate_param_count << ".\n";
  std::cout << "Unique accurate params: " << unique_accurate_param_count << ".\n";
  std::cout << "Total per-voice accurate params: " << total_voice_accurate_param_count << ".\n";
  std::cout << "Unique per-voice accurate params: " << unique_voice_accurate_param_count << ".\n";
}

void
generate_plugin_ref(
  plugin_topo const& topo, std::ostream& out)
{
  std::string name_and_version = topo.tag.full_name + " " + 
    std::to_string(topo.version.major) + "." + 
    std::to_string(topo.version.minor) + "." +
    std::to_string(topo.version.patch) ;

  std::string css = "";
  css += "th, td { padding: 3px; }";
  css += "a, a:visited { color: #666666; }";
  css += ".description { background:#EEEEEE; }";
  css += "h1 { font-size: 19px; color: black; }";
  css += "h2 { font-size: 17px; color: black; }";
  css += "h3 { font-size: 15px; color: black; }";
  css += "body { font-family: Verdana; color: black; }";
  css += "html { position: relative; max-width: 1024px; margin: auto; }";
  css += "tr td { width: auto; white-space: nowrap; } tr th { width: auto; white-space: nowrap; }";
  css += "table, th, td { font-size: 13px; border: 1px solid gray; border-collapse: collapse; text-align: left; }";
  css += "tr td:last-child { width: 100%; white-space: wrap; } tr th:last-child { width: 100%; white-space: wrap; }";

  out << "<html>\n";
  out << "<head>\n";
  out << "<title>" << name_and_version << "</title>\n";
  out << "<style>" + css + "</style>\n";
  out << "</head>\n";
  out << "<body>\n";
  out << "<h1>" << name_and_version << "</h1>\n";
  generate_modules_ref(topo, out);
  generate_params_ref(topo, out);
  out << "</body>\n";
  out << "</html>\n";
}

static void
generate_modules_ref(
  plugin_topo const& topo, std::ostream& out)
{
  out << "<h2>Module Overview</h2>\n";
  out << "<table>\n";
  out << "<tr>\n";
  out << "<th>#</th>\n";
  out << "<th>Name</th>\n";
  out << "<th>UI</th>\n";
  out << "<th>Stage</th>\n";
  out << "<th>Num</th>\n";
  out << "<th>Description</th>\n";
  out << "</tr>\n";

  for(int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    assert(module.info.description.size());
    assert(module.info.tag.full_name.size());

    out << "<tr>\n";
    if(module.gui.visible)
      out << "<td><a href='#" + std::to_string(m + 1) + "'>" + std::to_string(m + 1) + "</a></td>\n";
    else
      out << "<td>" + std::to_string(m + 1) + "</td>\n";
    out << "<td>" << module.info.tag.full_name << "</td>\n";
    out << "<td>" << (module.gui.visible? "Yes": "No") << "</td>\n";
    out << "<td>" << (module.dsp.stage == module_stage::voice
      ? "Voice" : module.dsp.stage == module_stage::input
      ? "Before voice": "After voice") << "</td>\n";
    out << "<td>" << module.info.slot_count << "</td>\n";
    out << "<td class='description'>" << module.info.description << "</td>\n";
    out << "</tr>\n";
  }

  out << "</table>\n";
}

static void
generate_params_ref(
  plugin_topo const& topo, std::ostream& out)
{
  out << "<h2>Parameter Overview</h2>\n";
  for(int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    if (module.gui.visible)
    {
      out << "<a name='" + std::to_string(m + 1) + "'/>\n";
      out << "<h3>" << module.info.tag.full_name << "</h3>\n";
      out << "<table>\n";
      out << "<tr>\n";
      out << "<th>Name</th>\n";
      out << "<th>Section</th>\n";
      out << "<th>UI</th>\n";
      out << "<th>Num</th>\n";
      out << "<th>Dir</th>\n";
      out << "<th>Rate</th>\n";
      out << "<th>Automate</th>\n";
      out << "<th>Min</th>\n";
      out << "<th>Max</th>\n";
      out << "<th>Default</th>\n";
      out << "<th>Log Midpoint</th>\n";
      out << "</tr>\n";

      int reference_module_slot = module.info.slot_count == 1 ? 0 : 1;
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        assert(param.info.description.size());
        assert(param.info.tag.full_name.size());
        int reference_param_slot = param.info.slot_count == 1 ? 0 : 1;

        std::string direction = "";
        switch (param.dsp.direction)
        {
        case param_direction::input: direction = "Input"; break;
        case param_direction::output: direction = "Output"; break;
        default: assert(false); break;
        }

        std::string rate = "";
        switch (param.dsp.rate)
        {
        case param_rate::block: rate = "Block"; break;
        case param_rate::voice: rate = "Voice"; break;
        case param_rate::accurate: rate = "Accurate"; break;
        default: assert(false); break;
        }

        std::string automate = "";
        auto automate_value = param.dsp.automate_selector_(reference_module_slot);
        switch (automate_value)
        {
        case param_automate::none: automate = "None"; break;
        case param_automate::midi: automate = "MIDI"; break;
        case param_automate::automate: automate = "Automate"; break;
        case param_automate::modulate: automate = "Modulate"; break;
        default: assert(false); break;
        }

        std::string min_value = param.domain.raw_to_text(false, param.domain.min);
        std::string max_value = param.domain.raw_to_text(false, param.domain.max);
        std::string default_value = param.domain.raw_to_text(false, param.domain.default_raw(reference_module_slot, reference_param_slot));

        std::string log_mid_value = "N/A";
        if(param.domain.type == domain_type::log)
          log_mid_value = param.domain.raw_to_text(false, param.domain.midpoint);

        out << "<tr>\n";
        out << "<td rowspan='2'>" << param.info.tag.full_name << "</td>\n";
        out << "<td>" << module.sections[param.gui.section].tag.full_name << "</td>\n";
        out << "<td>" << (param.gui.visible? "Yes": "No") << "</td>\n";
        out << "<td>" << param.info.slot_count << "</td>\n";
        out << "<td>" << direction << "</td>\n";
        out << "<td>" << rate << "</td>\n";
        out << "<td>" << automate << "</td>\n";
        out << "<td>" << min_value << "</td>\n";
        out << "<td>" << max_value << "</td>\n";
        out << "<td>" << default_value << "</td>\n";
        out << "<td>" << log_mid_value << "</td>\n";
        out << "</tr>\n";
        out << "<tr>\n";
        out << "<td colspan='11' class='description'>" << param.info.description << "</td>\n";
        out << "</tr>\n";
      }
      out << "</table>\n";
    }
  }
}

}