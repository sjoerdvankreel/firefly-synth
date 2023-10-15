#include <plugin_base/shared/notifier.hpp>

namespace plugin_base {

void
plugin_notifier::plugin_changed(int index, plain_value plain)
{
  auto iter = _listeners.find(index);
  if(iter == _listeners.end()) return;
  for(int i = 0; i < iter->second.size(); i++)
    iter->second[i]->plugin_changed(index, plain);
}

void
plugin_notifier::remove_listener(int index, plugin_listener* listener)
{
  auto map_iter = _listeners.find(index);
  assert(map_iter != _listeners.end());
  auto vector_iter = std::find(map_iter->second.begin(), map_iter->second.end(), listener);
  assert(vector_iter != map_iter->second.end());
  map_iter->second.erase(vector_iter);
}

}