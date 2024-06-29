#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

#if (defined WIN32)
#include <Windows.h>
#elif (defined __linux__) || (defined __FreeBSD__) || (defined __APPLE__)
#include <dlfcn.h>
#else
#error
#endif

// autogenerates module and parameter reference based on topology
// must match declarations in plugin_base
struct plugin_topo;
typedef plugin_topo const*(*pb_plugin_topo_create)();
typedef void(*pb_plugin_topo_destroy)(plugin_topo const*);
typedef void(*pb_plugin_topo_on_reference_generated)(char const* c, void* ctx);
typedef void (*pb_plugin_topo_generate_reference)(
  plugin_topo const* topo, pb_plugin_topo_on_reference_generated on_generated, void* ctx);

#if (defined WIN32)
static void* 
load_library(char const* path)
{ return LoadLibraryA(path); }
static void
close_library(void* handle)
{ FreeLibrary((HMODULE)handle); }
static void*
library_get_address(void* handle, char const* sym)
{ return GetProcAddress((HMODULE)handle, sym); }
#elif (defined __linux__) || (defined __FreeBSD__) || defined (__APPLE__)
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

int 
main(int argc, char** argv)
{
  std::ofstream out;
  std::string err = "";
  void* library = nullptr;
  plugin_topo const* topo = nullptr;

  void* topo_create = nullptr;
  void* topo_destroy = nullptr;
  void* topo_generate_reference = nullptr;

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

  if ((topo_generate_reference = library_get_address(library, "pb_plugin_topo_generate_reference")) == nullptr) {
    err = "Library does not export pb_plugin_topo_generate_reference.";
    goto end;
  }

  if ((topo = ((pb_plugin_topo_create)(topo_create))()) == nullptr) {
    err = "Failed to create topology.";
    goto end; }
  
  out.open(argv[2]);
  if (out.bad()) {
    err = "Failed to open output file.";
    goto end;
  }

  ((pb_plugin_topo_generate_reference)topo_generate_reference)(topo,
    [](char const* text, void* ctx) { 
      (*static_cast<std::ofstream*>(ctx)) << text; }, 
   &out);
  out.flush();

end:  
  if(topo != nullptr)
    ((pb_plugin_topo_destroy)(topo_destroy))(topo);
  if(library != nullptr)
    close_library(library);
  std::cout << err << std::endl;
  return err == "" ? 0 : 1;
}