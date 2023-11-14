#include <plugin_base/shared/io_user.hpp>

#include <juce_core/juce_core.h>

#include <algorithm>
#include <filesystem>

using namespace juce;

namespace plugin_base {

static std::string
user_location(plugin_topo const& topo)
{
  std::filesystem::path result = topo.vendor;
  result /= topo.tag.name;
  result /= topo.tag.id;
  result /= std::to_string(topo.version_major) + "." + std::to_string(topo.version_minor);
  return result.string();
}

static std::unique_ptr<InterProcessLock>
user_lock(plugin_topo const& topo)
{
  auto name = String(user_location(topo));
  return std::make_unique<InterProcessLock>(name);
}

static PropertiesFile::Options
user_options(plugin_topo const& topo, InterProcessLock* lock)
{
  PropertiesFile::Options result;
  result.processLock = lock;
  result.filenameSuffix = ".xml";
  result.applicationName = "plugin";
  result.folderName = user_location(topo);
  result.storageFormat = PropertiesFile::StorageFormat::storeAsXML;
  return result;
}

template <class UserAction>
static auto 
user_action(plugin_topo const& topo, user_io where, std::string const& key, UserAction action)
{
  auto lock(user_lock(topo));
  ApplicationProperties props;
  props.setStorageParameters(user_options(topo, lock.get()));
  std::string location = where == user_io::base? "base": "plugin";
  location += "_" + key;
  return action(props.getUserSettings(), location);
}

void
user_io_save_num(plugin_topo const& topo, user_io where, std::string const& key, double val)
{
  auto action = [val](auto store, auto const& k) { return store->setValue(k, var(val)); };
  return user_action(topo, where, key, action);
}

void
user_io_save_text(plugin_topo const& topo, user_io where, std::string const& key, std::string const& val)
{
  auto action = [val](auto store, auto const& k) { return store->setValue(k, var(val)); };
  return user_action(topo, where, key, action);
}

std::string 
user_io_load_text(plugin_topo const& topo, user_io where, std::string const& key, std::string const& default_)
{
  auto action = [default_](auto store, auto const& k) { return store->getValue(k, default_); };
  return user_action(topo, where, key, action).toStdString();
}

double
user_io_load_num(plugin_topo const& topo, user_io where, std::string const& key, double default_, double min, double max)
{
  auto action = [default_](auto store, auto const& k) { return store->getDoubleValue(k, default_); };
  return std::clamp(user_action(topo, where, key, action), min, max);
}

}