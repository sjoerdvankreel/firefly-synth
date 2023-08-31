#include <infernal.base/plugin_engine.hpp>

#include <limits>
#include <utility>

namespace infernal::base {

plugin_engine::
plugin_engine(plugin_topo&& topo) : _topo(std::move(topo))
{
  _host_block.common.notes.reserve(_topo.static_topo.note_limit);
  _host_block.block_automation.reserve(_topo.static_topo.block_automation_limit);
  _host_block.accurate_automation.reserve(_topo.static_topo.accurate_automation_limit);
  _accurate_automation_frames.resize(_topo.runtime_params.size());

  _plugin_block.host = &_host_block.common;
  auto const& static_mods = _topo.static_topo.modules;
  int mod_type_count = static_mods.size();
  _state = new param_value**[mod_type_count]();
  _plugin_block.module_cv = new float**[mod_type_count]();
  _plugin_block.module_audio = new float***[mod_type_count]();
  _plugin_block.accurate_automation = new float*** [mod_type_count]();
  _plugin_block.block_automation = new param_value**[mod_type_count]();

  for (int m = 0; m < static_mods.size(); m++)
  {
    int mod_count = static_mods[m].count;
    _state[m] = new param_value*[mod_count]();
    _plugin_block.module_cv[m] = new float*[mod_count]();
    _plugin_block.module_audio[m] = new float**[mod_count]();
    _plugin_block.accurate_automation[m] = new float** [mod_count]();
    _plugin_block.block_automation[m] = new param_value*[mod_count]();

    for (int i = 0; i < static_mods[m].count; i++)
    {
      auto const& flat_mod = _topo.flat_modules[m];
      int mod_param_count = flat_mod.params.size();
      _state[m][i] = new param_value[mod_param_count];
      for(int p = 0; p < mod_param_count; p++)
        _state[m][i][flat_mod.params[p]->type] = flat_mod.params[p]->default_value();

      if(static_mods[m].output == module_output::audio)
        _plugin_block.module_audio[m][i] = new float* [_topo.static_topo.channel_count]();
      _plugin_block.accurate_automation[m][i] = new float* [mod_param_count]();
      _plugin_block.block_automation[m][i] = new param_value[mod_param_count]();
    }
  }
}

plugin_engine::
~plugin_engine()
{
  for (int m = 0; m < _topo.flat_modules.size(); m++)
  {
    auto const& flat_mod_topo = _topo.flat_modules[m].static_topo;
    for (int i = 0; i < flat_mod_topo->count; i++)
    {
      delete _state[m][i];
      if (flat_mod_topo->output == module_output::audio)
        delete _plugin_block.module_audio[m][i];
      delete _plugin_block.block_automation[m][i];
      delete _plugin_block.accurate_automation[m][i];
    }

    delete _state[m];
    delete _plugin_block.module_cv[m];
    delete _plugin_block.module_audio[m];
    delete _plugin_block.block_automation[m];
    delete _plugin_block.accurate_automation[m];
  }

  delete _state;
  delete _plugin_block.module_cv;
  delete _plugin_block.module_audio;
  delete _plugin_block.block_automation;
  delete _plugin_block.accurate_automation;
}

void
plugin_engine::activate(int sample_rate, int max_frame_count)
{
  _sample_rate = sample_rate;
  for (int m = 0; m < _topo.flat_modules.size(); m++)
  {
    auto const& flat_mod = _topo.flat_modules[m].static_topo;
    for (int i = 0; i < flat_mod->count; i++)
    {
      if (flat_mod->output == module_output::cv)
        _plugin_block.module_cv[m][i] = new float[max_frame_count]();
      else if (flat_mod->output == module_output::audio)
        for(int c = 0; c < _topo.static_topo.channel_count; c++)
          _plugin_block.module_audio[m][i][c] = new float[max_frame_count]();
      for(int p = 0; p < _topo.flat_modules[m].params.size(); p++)
        _plugin_block.accurate_automation[m][i][p] = new float[max_frame_count]();
    }
  }
}

void
plugin_engine::deactivate()
{
  _sample_rate = 0;
  for (int m = 0; m < _topo.flat_modules.size(); m++)
  {
    auto const& flat_mod = _topo.flat_modules[m].static_topo;
    for (int i = 0; i < flat_mod->count; i++)
    {
      if (flat_mod->output == module_output::cv)
      {
        delete _plugin_block.module_cv[m][i];
        _plugin_block.module_cv[m][i] = nullptr;
      }
      else if (flat_mod->output == module_output::audio)
        for (int c = 0; c < _topo.static_topo.channel_count; c++)
        {
          delete _plugin_block.module_audio[m][i][c];
          _plugin_block.module_audio[m][i][c] = nullptr;
        }
      for (int p = 0; p < _topo.flat_modules[m].params.size(); p++)
      {
        delete _plugin_block.accurate_automation[m][i][p];
        _plugin_block.accurate_automation[m][i][p] = nullptr;
      }
    }
  }
}

host_block& 
plugin_engine::prepare()
{
  _host_block.common.bpm = 0;
  _host_block.common.frame_count = 0;
  _host_block.common.stream_time = 0;
  _host_block.common.audio_input = nullptr;
  _host_block.common.audio_output = nullptr;
  _host_block.common.notes.clear();
  _host_block.block_automation.clear();
  _host_block.accurate_automation.clear();
  return _host_block;
}

void 
plugin_engine::process()
{
  for(int m = 0; m < _topo.static_topo.modules.size(); m++)
  {
    auto const& flat_mod = _topo.flat_modules[m].static_topo;
    if (flat_mod->output == module_output::cv)
      for(int i = 0; i < flat_mod->count; i++)
        std::fill(
          _plugin_block.module_cv[m][i], 
          _plugin_block.module_cv[m][i] + _host_block.common.frame_count,
          std::numeric_limits<float>::quiet_NaN());
    else if(flat_mod->output == module_output::audio)
      for (int i = 0; i < flat_mod->count; i++)
        for(int c = 0; c < _topo.static_topo.channel_count; c++)
          std::fill(
            _plugin_block.module_audio[m][i][c], 
            _plugin_block.module_audio[m][i][c] + _host_block.common.frame_count,
            std::numeric_limits<float>::quiet_NaN());
  }

  for(int p = 0; p < _topo.runtime_params.size(); p++)
  {
    auto const& rt_param = _topo.runtime_params[p];
    int mt = rt_param.module_type;
    int mi = rt_param.module_index;
    int pi = rt_param.module_param_index;
    int pt = rt_param.static_topo->type;
    if (_topo.flat_modules[mt].params[pi]->rate == param_rate::block)
      _plugin_block.block_automation[mt][mi][pt] = _state[mt][mi][pt];
    else
      std::fill(
        _plugin_block.accurate_automation[mt][mi][pt],
        _plugin_block.accurate_automation[mt][mi][pt] + _host_block.common.frame_count,
        _state[mt][mi][pt].real);
  }
    
  for (int a = 0; a < _host_block.block_automation.size(); a++)
  {
    auto const& event = _host_block.block_automation[a];
    auto const& rt_param = _topo.runtime_params[event.runtime_param_index];
    int mt = rt_param.module_type;
    int mi = rt_param.module_index;
    int pt = rt_param.static_topo->type;
    _state[mt][mi][pt] = event.value;
    _plugin_block.block_automation[mt][mi][pt] = event.value;
  }

  std::fill(_accurate_automation_frames.begin(), _accurate_automation_frames.end(), 0);
  for (int a = 0; a < _host_block.accurate_automation.size(); a++)
  {
    auto const& event = _host_block.accurate_automation[a];
    auto rpi = event.runtime_param_index;
    auto const& rt_param = _topo.runtime_params[rpi];
    int mt = rt_param.module_type;
    int mi = rt_param.module_index;
    int pt = rt_param.static_topo->type;
    int prev_frame = _accurate_automation_frames[rpi];
    float frame_count = event.frame_index - prev_frame;
    float start = _plugin_block.accurate_automation[mt][mi][pt][_accurate_automation_frames[rpi]];
    float range = event.value - start;
    for(int f = prev_frame; f < event.frame_index; f++)
      _plugin_block.accurate_automation[mt][mi][pt][f] = start + (f - prev_frame) / frame_count * range;
    _state[mt][mi][pt].real = event.value;
    _accurate_automation_frames[rpi] = event.frame_index;
  }

  for(int m = 0; m < _topo.static_topo.modules.size(); m++)
  {
    auto const& static_mod = _topo.static_topo.modules[m];
    for(int i = 0; i < static_mod.count; i++)
      static_mod.callbacks.process(static_mod, i, _plugin_block);
  }
}

}