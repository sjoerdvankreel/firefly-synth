#include <plugin_base/block_plugin.hpp>
#include <algorithm>

namespace plugin_base {

void
process_block::set_out_param(int param, int slot, double raw)
{
  plain_value plain = module.params[param].topo->raw_to_plain(raw);
  block.out->output_params[param][slot] = out_gain;
}

}
