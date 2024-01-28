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
    err = "Usage: param_ref_gen path_to_library path_to_output.html."; 
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
  std::string title = name_and_version + " Reference";
  out << "<html>\n";
  out << "<head>\n";
  out << "<title>" << title << "</title>\n";
  out << "<style>html { position: relative; width: 1024px; margin: auto; } body { font-family: Verdana; } table, th, td { border: 1px solid gray; border-collapse: collapse; text-align: left; } th, td { padding: 3px; }</style>\n";
  out << "</head>\n";
  out << "<body>\n";
  generate_modules_ref(topo, out);
  out << "</body>\n";
  out << "</html>\n";
}

static void
generate_modules_ref(plugin_topo const& topo, std::ostream& out)
{
  out << "<h1>Module Overview</h1>\n";
  out << "<table>\n";
  out << "<tr>\n";
  out << "<th>Name</th>\n";
  out << "<th>Short name</th>\n";
  out << "<th>Stage</th>\n";
  out << "<th>Count</th>\n";
  out << "<th>Description</th>\n";
  out << "</tr>\n";

  auto sorted_modules = topo.modules;
  std::sort(sorted_modules.begin(), sorted_modules.end(), [](auto const& l, auto const& r) { return l.info.tag.name < r.info.tag.name; });
  for(int m = 0; m < sorted_modules.size(); m++)
    if(sorted_modules[m].gui.visible)
    {
      out << "<tr>\n";
      out << "<td>" << sorted_modules[m].info.tag.name << "</td>\n";
      out << "<td>" << sorted_modules[m].info.tag.short_name << "</td>\n";
      out << "<td>" << (sorted_modules[m].dsp.stage == module_stage::voice ? "Voice" : "Global") << "</td>\n";
      out << "<td>" << sorted_modules[m].info.slot_count << "</td>\n";
      out << "<td>" << sorted_modules[m].info.description << "</td>\n";
      out << "</tr>\n";
    }

  out << "</table>\n";
}
