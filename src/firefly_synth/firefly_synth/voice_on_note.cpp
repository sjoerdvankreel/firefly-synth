#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { output_on_note, output_count };

class voice_on_note_engine :
public module_engine {
  std::vector<module_output_mapping> const _global_outputs;
public:
  void process(plugin_block& block) override;
  void reset(plugin_block const* block) override {}

  PB_PREVENT_ACCIDENTAL_COPY(voice_on_note_engine);
  voice_on_note_engine(std::vector<module_output_mapping> const& global_outputs) : 
  _global_outputs(global_outputs) {}
};

module_topo
voice_on_note_topo(plugin_topo const* topo, int section)
{
  std::vector<module_dsp_output> outputs;
  std::string const on_note_id("{68360340-68B2-4B88-95BD-B1929F240BAA}");
  std::set<int> short_name_modules = { module_master_cv };
  auto global_sources(make_cv_source_matrix(make_cv_matrix_sources(topo, true, short_name_modules)));
  for(int i = 0; i < global_sources.items.size(); i++)
    outputs.push_back(make_module_dsp_output(true, make_topo_info(
      global_sources.items[i].id,
      global_sources.items[i].name, i, 1)));

  module_topo result(make_module(
    make_topo_info("{EF1A4E73-BCAD-4D38-A54E-44B83EF46CB5}", "On Note", "On Nt", true, module_voice_on_note, 1),
    make_module_dsp(module_stage::voice, module_output::cv, 0, outputs), make_module_gui_none(section)));
  result.engine_factory = [gm = global_sources.mappings](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<voice_on_note_engine>(gm); };
  return result;
}

void
voice_on_note_engine::process(plugin_block& block)
{  
}

}
