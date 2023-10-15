#pragma once

#include <plugin_base/shared/value.hpp>

namespace plugin_base {

class plugin_listener
{
public:
  virtual void plugin_changed(int index, plain_value plain) = 0;
};

}