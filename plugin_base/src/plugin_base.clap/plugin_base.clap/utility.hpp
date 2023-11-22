#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/value.hpp>

#include <readerwriterqueue.h>

#include <memory>
#include <cstdint>

namespace plugin_base::clap {

inline int constexpr default_q_size = 4096;
typedef std::unique_ptr<plugin_topo>(*topo_factory)();
enum class sync_event_type { end_edit, begin_edit, value_changing };

// sync audio <-> ui
struct sync_event
{
  int index = {};
  sync_event_type type;
  plain_value plain = {};
};

// linear and log map to normalized, step maps to plain
class clap_value {
  double _value;
public:
  clap_value(clap_value const&) = default;
  clap_value& operator=(clap_value const&) = default;
  inline double value() const { return _value; }
  explicit clap_value(double value) : _value(value) {}
};

}
