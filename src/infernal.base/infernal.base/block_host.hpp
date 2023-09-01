#pragma once
#include <infernal.base/utility.hpp>
#include <infernal.base/block_common.hpp>
#include <vector>

namespace infernal::base {

struct host_block_event final {
  double normalized;
  int runtime_param_index;
};

struct host_accurate_event final {
  double normalized;
  int frame_index;
  int runtime_param_index;
};

struct host_block final {
  common_block common;
  std::vector<host_block_event> block_events;
  std::vector<host_accurate_event> accurate_events;
  INF_DECLARE_MOVE_ONLY(host_block);
};

}
