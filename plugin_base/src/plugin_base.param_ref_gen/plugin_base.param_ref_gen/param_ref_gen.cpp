#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/utility.hpp>

#include <fstream>
#include <iostream>
#include <filesystem>

#if (defined WIN32)
#include <Windows.h>
#elif (defined __linux__) || (defined __FreeBSD__)
#else
#include <dlfcn.h>
#error
#endif

typedef plugin_base::plugin_topo const*(*pb_plugin_topo_create)();
typedef void(*pb_plugin_topo_destroy)(plugin_base::plugin_topo const*);

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
generate_param_ref(plugin_base::plugin_topo const& topo, std::ostream& out);

int 
main(int argc, char** argv)
{
  std::ofstream out;
  std::string err = "Success";
  void* library = nullptr;
  void* topo_create = nullptr;
  void* topo_destroy = nullptr;
  plugin_base::plugin_topo const* topo = nullptr;

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
    err = "Library does not export pb_plugin_topo_create.";
    goto end; }

  if ((topo = ((pb_plugin_topo_create)(topo_create))()) == nullptr) {
    err = "Failed to create topology.";
    goto end; }
  
  out.open(argv[2]);
  if (out.bad()) {
    err = "Failed to open output file.";
    goto end;
  }

  generate_param_ref(*topo, out);
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
generate_param_ref(plugin_base::plugin_topo const& topo, std::ostream& out)
{
  std::cout << topo.modules.size() << "\n";
  out << topo.tag.name;
}