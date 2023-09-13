#include <plugin_base/io.hpp>
#include <juce_core/juce_core.h>

using namespace juce;

namespace plugin_base {

void 
io_store_file(
  plugin_topo const& topo, 
  jarray4d<plain_value> const& state, 
  std::string const& path)
{
  return;
}

bool 
io_load_file(
  plugin_topo const& topo, 
  std::string const& path, 
  jarray4d<plain_value>& state)
{
  return {};
}

std::vector<std::uint8_t> 
io_store(
  plugin_topo const& topo, 
  jarray4d<plain_value> const& state)
{
  DynamicObject root;
  root.setProperty("id", var(String(topo.id)));
  root.setProperty("version_major", var(topo.version_major));
  root.setProperty("version_minor", var(topo.version_major));
  std::string json = var(&root).toString().toStdString();
  return {};
}


bool 
io_load(
  plugin_topo const& topo, 
  std::vector<std::uint8_t> const& blob, 
  jarray4d<plain_value>& state)
{
  return {};
}

}