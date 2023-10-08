#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <plugin_base.clap/inf_plugin.hpp>

#include <cstring>

namespace plugin_base::clap {

template <
  clap_plugin_descriptor_t const* Descriptor,
  std::unique_ptr<plugin_topo>(*TopoFactory)()>
void const* 
get_plugin_factory(char const* factory_id)
{
  if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID)) return nullptr;
  static clap_plugin_factory result;
  result.get_plugin_count = [](clap_plugin_factory const*) { return 1u; };
  result.get_plugin_descriptor = [](clap_plugin_factory const*, std::uint32_t) { return Descriptor; };
  result.create_plugin = [](clap_plugin_factory const*, clap_host const* host, char const*) 
  { return (new inf_plugin(Descriptor, host, TopoFactory()))->clapPlugin(); };
  return &result;
}

}