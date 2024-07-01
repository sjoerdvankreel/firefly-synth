#include <plugin_base/shared/io_user.hpp>
#include <plugin_base/shared/io_shared.hpp>
#include <juce_core/juce_core.h>

#include <cstddef>
#include <algorithm>
#include <filesystem>

using namespace juce;

namespace plugin_base {

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
  result.applicationName = "user_state";
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
user_io_save_list(plugin_topo const& topo, user_io where, std::string const& key, std::string const& val)
{
  auto action = [val](auto store, auto const& k) { return store->setValue(k, var(val)); };
  return user_action(topo, where, key, action);
}

double
user_io_load_num(plugin_topo const& topo, user_io where, std::string const& key, double default_, double min, double max)
{
  auto action = [default_](auto store, auto const& k) { return store->getDoubleValue(k, default_); };
  return std::clamp(user_action(topo, where, key, action), min, max);
}

std::string
user_io_load_list(plugin_topo const& topo, user_io where, std::string const& key, std::string const& default_, std::vector<std::string> const& values)
{
  auto action = [default_, values](auto store, auto const& k) { 
    auto result = store->getValue(k, default_);
    bool found_result = false;
    bool found_default = false;
    for (int i = 0; i < values.size(); i++)
    {
      found_result |= values[i] == result;
      found_default |= values[i] == default_;
    }
    if(!found_result) result = default_;
    return result;
  };
  return user_action(topo, where, key, action).toStdString();
}

}