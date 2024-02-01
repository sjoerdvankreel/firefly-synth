#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/utility.hpp>

#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

#if (defined WIN32)
#include <Windows.h>
#elif (defined __linux__) || (defined __FreeBSD__)
#include <dlfcn.h>
#else
#error
#endif

// autogenerates module and parameter reference based on topology

using namespace plugin_base;
typedef plugin_topo const*(*pb_plugin_topo_create)();
typedef void(*pb_plugin_topo_destroy)(plugin_topo const*);

#if (defined WIN32)
static void* 
load_library(char const* path)
{ return LoadLibraryA(path); }
static void
close_library(void* handle)
{ PB_ASSERT_EXEC(FreeLibrary((HMODULE)handle)); }
static void*
library_get_address(void* handle, char const* sym)
{ return GetProcAddress((HMODULE)handle, sym); }
#elif (defined __linux__) || (defined __FreeBSD__)
static void* 
load_library(char const* path)
{ return dlopen(path, RTLD_NOW); }
static void
close_library(void* handle)
{ PB_ASSERT_EXEC(dlclose(handle) == 0); }
static void*
library_get_address(void* handle, char const* sym)
{ return dlsym(handle, sym); }
#else
#error
#endif

static void 
generate_plugin_ref(plugin_topo const& topo, std::ostream& out);
static void
generate_params_ref(plugin_topo const& topo, std::ostream& out);
static void
generate_modules_ref(plugin_topo const& topo, std::ostream& out);

int 
main(int argc, char** argv)
{
  std::ofstream out;
  std::string err = "Success";
  void* library = nullptr;
  void* topo_create = nullptr;
  void* topo_destroy = nullptr;
  plugin_topo const* topo = nullptr;

  if(argc != 3) {
    err = "Usage: ref_gen path_to_library path_to_output.html."; 
    goto end; }
  
  if(!std::filesystem::exists(argv[1])) {
    err = "Library not found.";
    goto end; }

  if((library = load_library(argv[1])) == nullptr) { 
    err = "Failed to load library.";
    goto end; }

  if((topo_create = library_get_address(library, "pb_plugin_topo_create")) == nullptr) {
    err = "Library does not export pb_plugin_topo_create.";
    goto end; }

  if ((topo_destroy = library_get_address(library, "pb_plugin_topo_destroy")) == nullptr) {
    err = "Library does not export pb_plugin_topo_destroy.";
    goto end; }

  if ((topo = ((pb_plugin_topo_create)(topo_create))()) == nullptr) {
    err = "Failed to create topology.";
    goto end; }
  
  out.open(argv[2]);
  if (out.bad()) {
    err = "Failed to open output file.";
    goto end;
  }

  generate_plugin_ref(*topo, out);
  out.flush();

end:  
  if(topo != nullptr)
    ((pb_plugin_topo_destroy)(topo_destroy))(topo);
  if(library != nullptr)
    close_library(library);
  std::cout << err << std::endl;
  return err == "Success" ? 0 : 1;
}

void
generate_plugin_ref(plugin_topo const& topo, std::ostream& out)
{
  std::string name_and_version = topo.tag.name + " " + 
    std::to_string(topo.version_major) + "." + 
    std::to_string(topo.version_minor);
  
  std::string title = name_and_version + " Reference Manual";
  std::string foreground = "#" + topo.gui.colors.control_text.toDisplayString(false).toStdString();
  std::string background = "#" + topo.gui.colors.control_background.toDisplayString(false).toStdString();
  std::string color1 = "#" + topo.gui.reference_colors.color1.toDisplayString(false).toStdString();
  std::string color2 = "#" + topo.gui.reference_colors.color2.toDisplayString(false).toStdString();
  std::string color3 = "#" + topo.gui.reference_colors.color3.toDisplayString(false).toStdString();
  std::string color4 = "#" + topo.gui.reference_colors.color4.toDisplayString(false).toStdString();

  std::string css = "";
  css += "th, td { padding: 3px; }";
  css += "th, a, a:visited { color: " + color4 + "; }";
  css += "h1 { font-size: 19px; color: " + color1 + "; }";
  css += "h2 { font-size: 17px; color: " + color2 + "; }";
  css += "h3 { font-size: 15px; color: " + color3 + "; }";
  css += "html { position: relative; max-width: 1440px; margin: auto; }";
  css += "body { font-family: Verdana; background: " + background + "; color: " + foreground + "; }";
  css += "tr td { width: auto; white-space: nowrap; } tr th { width: auto; white-space: nowrap; }";
  css += "table, th, td { font-size: 13px; border: 1px solid gray; border-collapse: collapse; text-align: left; }";
  css += "tr td:last-child { width: 100%; white-space: wrap; } tr th:last-child { width: 100%; white-space: wrap; }";

  out << "<html>\n";
  out << "<head>\n";
  out << "<title>" << title << "</title>\n";
  out << "<style>" + css + "</style>\n";
  out << "</head>\n";
  out << "<body>\n";
  out << "<h1>" << title << "</h1>\n";
  generate_modules_ref(topo, out);
  generate_params_ref(topo, out);
  out << "</body>\n";
  out << "</html>\n";
}

static void
generate_modules_ref(plugin_topo const& topo, std::ostream& out)
{
  out << "<h2>Module Overview</h2>\n";
  out << "<table>\n";
  out << "<tr>\n";
  out << "<th>#</th>\n";
  out << "<th>Name</th>\n";
  out << "<th>Short</th>\n";
  out << "<th>UI</th>\n";
  out << "<th>Stage</th>\n";
  out << "<th>Num</th>\n";
  out << "<th>Description</th>\n";
  out << "</tr>\n";

  for(int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    out << "<tr>\n";
    if(module.gui.visible)
      out << "<td><a href='#" + std::to_string(m + 1) + "'>" + std::to_string(m + 1) + "</a></td>\n";
    else
      out << "<td>" + std::to_string(m + 1) + "</td>\n";
    out << "<td>" << module.info.tag.name << "</td>\n";
    out << "<td>" << module.info.tag.short_name << "</td>\n";
    out << "<td>" << (module.gui.visible? "Yes": "No") << "</td>\n";
    out << "<td>" << (module.dsp.stage == module_stage::voice
      ? "Voice" : module.dsp.stage == module_stage::input
      ? "Before voice": "After voice") << "</td>\n";
    out << "<td>" << module.info.slot_count << "</td>\n";
    out << "<td>" << module.info.description << "</td>\n";
    out << "</tr>\n";
  }

  out << "</table>\n";
}

static void
generate_params_ref(plugin_topo const& topo, std::ostream& out)
{
  out << "<h2>Parameter Overview</h2>\n";
  for(int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module = topo.modules[m];
    if (module.gui.visible)
    {
      out << "<a name='" + std::to_string(m + 1) + "'/>\n";
      out << "<h3>" << module.info.tag.name << "</h3>\n";
      out << "<table>\n";
      out << "<tr>\n";
      out << "<th>Name</th>\n";
      out << "<th>Short</th>\n";
      out << "<th>Section</th>\n";
      out << "<th>UI</th>\n";
      out << "<th>Num</th>\n";
      out << "<th>Dir</th>\n";
      out << "<th>Rate</th>\n";
      out << "<th>Automate</th>\n";
      out << "<th>Min</th>\n";
      out << "<th>Max</th>\n";
      out << "<th>Default</th>\n";
      out << "<th>Log Mid</th>\n";
      out << "<th>Description</th>\n";
      out << "</tr>\n";

      int reference_module_slot = module.info.slot_count == 1 ? 0 : 1;
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        auto const& short_name = param.info.tag.short_name.size()? param.info.tag.short_name: param.info.tag.name;
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
        out << "<td>" << param.info.tag.name << "</td>\n";
        out << "<td>" << short_name << "</td>\n";
        out << "<td>" << module.sections[param.gui.section].tag.name << "</td>\n";
        out << "<td>" << (param.gui.visible? "Yes": "No") << "</td>\n";
        out << "<td>" << param.info.slot_count << "</td>\n";
        out << "<td>" << direction << "</td>\n";
        out << "<td>" << rate << "</td>\n";
        out << "<td>" << automate << "</td>\n";
        out << "<td>" << min_value << "</td>\n";
        out << "<td>" << max_value << "</td>\n";
        out << "<td>" << default_value << "</td>\n";
        out << "<td>" << log_mid_value << "</td>\n";
        out << "<td>" << param.info.description << "</td>\n";
        out << "</tr>\n";
      }
      out << "</table>\n";
    }
  }
}
