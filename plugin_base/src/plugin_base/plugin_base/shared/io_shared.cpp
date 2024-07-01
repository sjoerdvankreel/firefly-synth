#include <plugin_base/shared/io_shared.hpp>

namespace plugin_base {

std::string
user_location(plugin_topo const& topo)
{ return user_location(topo.vendor, topo.full_name); }

std::string
user_location(std::string const& vendor, std::string const& full_name)
{
  auto result = std::filesystem::path(vendor) / full_name;
#ifdef __linux__
  char const* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  if(xdg_config_home == nullptr) xdg_config_home = ".config";
  result = std::filesystem::path(xdg_config_home) / result;
#endif
  return result.string();
}

}