#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <array>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { type_off, type_time, type_sync };
enum { param_type, param_time, param_tempo, param_gain };

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{29E96CF3-6957-40EB-BE8D-6B3E9786B9A4}", "Off");
  result.emplace_back("{0B4D8D4C-1969-4659-9ED9-0BB9EE1AA5B6}", "Time");
  result.emplace_back("{6BD6BD00-A02B-478A-9463-8BAEC7A3BBFB}", "Sync");
  return result;
}

class delay_engine: 
public module_engine {
  int _pos;
  int const _capacity;
  jarray<float, 2> _buffer = {};

public:
  delay_engine(int sample_rate);
  INF_PREVENT_ACCIDENTAL_COPY(delay_engine);
  void initialize() override;
  void process(plugin_block& block) override;
};

module_topo
delay_topo(
  int section, plugin_base::module_gui_colors const& colors, 
  plugin_base::gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{ADA77C05-5D2B-4AA0-B705-A5BE89C32F37}", "Delay", module_delay, 1), 
    make_module_dsp(module_stage::output, module_output::none, 0, 1),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{05CF51D6-35F9-4115-A654-83EEE584B68E}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1 } })));

  result.params.emplace_back(make_param(
    make_topo_info("{A8638DE3-B574-4584-99A2-EC6AEE725839}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));

  auto& time = result.params.emplace_back(make_param(
    make_topo_info("{C39B97B3-B417-4C72-92C0-B8D764347792}", "Time", param_time, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(0.1, 2, 1, 2, "Sec"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  time.gui.bindings.enabled.bind({ param_type }, [](auto const& vs) { return vs[0] == type_time; });
  time.gui.bindings.visible.bind({ param_type }, [](auto const& vs) { return vs[0] != type_sync; });

  auto& tempo = result.params.emplace_back(make_param(
    make_topo_info("{D4A46363-DB92-425C-A9F7-D6641115812E}", "Tempo", param_tempo, 1),
    make_param_dsp_block(param_automate::automate), make_domain_timesig_default(),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 1 }, make_label_none())));
  tempo.gui.submenus = make_timesig_submenus(tempo.domain.timesigs);
  tempo.gui.bindings.visible.bind({ param_type }, [](auto const& vs) { return vs[0] == type_sync; });

  result.params.emplace_back(make_param(
    make_topo_info("{2E80A7CE-735B-48C4-8681-FBE1EE003297}", "Gain", param_gain, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.engine_factory = [](auto const&, int sample_rate, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<delay_engine>(sample_rate); };

  return result;
}

delay_engine::
delay_engine(int sample_rate) : 
  _capacity(sample_rate * 10)
{ 
  _buffer.resize(jarray<int, 1>(2, _capacity));
  initialize();
}

void
delay_engine::initialize()
{
  _pos = 0;
  _buffer.fill(0);
}

void
delay_engine::process(plugin_block& block)
{
  for (int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
      block.out->host_audio[c][f] = block.out->mixdown[c][f];
  int type = block.state.own_block_automation[param_type][0].step();
  if (type == type_off) return;

  auto const& time_curve = sync_or_time_into_scratch(block,
    type == type_sync, module_delay, param_time, param_tempo, 0);
  auto const& gain_curve = block.state.own_accurate_automation[param_gain][0];
  for (int c = 0; c < 2; c++)
    for(int f = block.start_frame; f < block.end_frame; f++)
    {
      int samples = block.sample_rate * time_curve[f];
      block.out->host_audio[c][f] = block.out->mixdown[c][f];
      block.out->host_audio[c][f] += _buffer[c][(_pos + f + _capacity - samples) % _capacity] * gain_curve[f];
      _buffer[c][(_pos + f) % _capacity] = block.out->mixdown[c][f];
    }
  _pos += block.end_frame - block.start_frame;
  _pos %= _capacity;
}

}
