#include <infernal.base/plugin_engine.hpp>
#include <limits>

namespace infernal::base {

plugin_engine::
plugin_engine(plugin_topo const& topo) : 
_topo(topo), _runtime_topo(topo)
{
  _host_block.notes.reserve(topo.note_limit);
  _host_block.block_automation.reserve(topo.block_automation_limit);
  _host_block.accurate_automation.reserve(topo.accurate_automation_limit);
  _accurate_automation_frames.resize(_runtime_topo.runtime_params.size());

  _plugin_block.host = &_host_block;
  int mod_type_count = _topo.modules.size();
  _plugin_block.module_cv = new float**[mod_type_count]();
  _plugin_block.module_audio = new float***[mod_type_count]();
  _plugin_block.accurate_automation = new float*** [mod_type_count]();
  _plugin_block.block_automation = new param_value**[mod_type_count]();

  for (int m = 0; m < _topo.modules.size(); m++)
  {
    int mod_count = _topo.modules[m].count;
    _state[m] = new param_value*[mod_count]();
    _plugin_block.module_cv[m] = new float*[mod_count]();
    _plugin_block.module_audio[m] = new float**[mod_count]();
    _plugin_block.accurate_automation[m] = new float** [mod_count]();
    _plugin_block.block_automation[m] = new param_value*[mod_count]();

    for (int i = 0; i < _topo.modules[m].count; i++)
    {
      int mod_param_count = _runtime_topo.module_params[m].size();
      _state[m][i] = new param_value[mod_param_count];
      for(int p = 0; p < mod_param_count; p++)
        _state[m][i][p] = _runtime_topo.module_params[m][p].default_value();

      if(_topo.modules[m].output == module_output::audio)
        _plugin_block.module_audio[m][i] = new float* [_topo.channel_count]();
      _plugin_block.accurate_automation[m][i] = new float* [mod_param_count]();
      _plugin_block.block_automation[m][i] = new param_value[mod_param_count]();
    }
  }
}

plugin_engine::
~plugin_engine()
{
  for (int m = 0; m < _topo.modules.size(); m++)
  {
    for (int i = 0; i < _topo.modules[m].count; i++)
    {
      delete _state[m][i];
      if (_topo.modules[m].output == module_output::audio)
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

  delete _plugin_block.module_cv;
  delete _plugin_block.module_audio;
  delete _plugin_block.block_automation;
  delete _plugin_block.accurate_automation;
}

void
plugin_engine::activate(int frame_count)
{
  for (int m = 0; m < _topo.modules.size(); m++)
    for (int i = 0; i < _topo.modules[m].count; i++)
    {
      if (_topo.modules[m].output == module_output::cv)
        _plugin_block.module_cv[m][i] = new float[frame_count]();
      else if (_topo.modules[m].output == module_output::audio)
        for(int c = 0; c < _topo.channel_count; c++)
          _plugin_block.module_audio[m][i][c] = new float[frame_count]();
      for(int p = 0; p < _runtime_topo.module_params[m].size(); p++)
        _plugin_block.accurate_automation[m][i][p] = new float[frame_count]();
    }
}

void
plugin_engine::deactivate()
{
  for (int m = 0; m < _topo.modules.size(); m++)
    for (int i = 0; i < _topo.modules[m].count; i++)
    {
      if (_topo.modules[m].output == module_output::cv)
      {
        delete _plugin_block.module_cv[m][i];
        _plugin_block.module_cv[m][i] = nullptr;
      }
      else if (_topo.modules[m].output == module_output::audio)
        for (int c = 0; c < _topo.channel_count; c++)
        {
          delete _plugin_block.module_audio[m][i][c];
          _plugin_block.module_audio[m][i][c] = nullptr;
        }
      for (int p = 0; p < _runtime_topo.module_params[m].size(); p++)
      {
        delete _plugin_block.accurate_automation[m][i][p];
        _plugin_block.accurate_automation[m][i][p] = nullptr;
      }
    }
}

void 
plugin_engine::process()
{
  _host_block.notes.clear();
  _host_block.block_automation.clear();
  _host_block.accurate_automation.clear();
  prepare(_host_block);

  for(int m = 0; m < _topo.modules.size(); m++)
    if (_topo.modules[m].output == module_output::cv)
      for(int i = 0; i < _topo.modules[m].count; i++)
        std::fill(
          _plugin_block.module_cv[m][i], 
          _plugin_block.module_cv[m][i] + _host_block.frame_count, 
          std::numeric_limits<float>::quiet_NaN());
    else if(_topo.modules[m].output == module_output::audio)
      for (int i = 0; i < _topo.modules[m].count; i++)
        for(int c = 0; c < _topo.channel_count; c++)
          std::fill(
            _plugin_block.module_audio[m][i][c], 
            _plugin_block.module_audio[m][i][c] + _host_block.frame_count, 
            std::numeric_limits<float>::quiet_NaN());

  for(int p = 0; p < _runtime_topo.runtime_params.size(); p++)
  {
    auto param_meta = _runtime_topo.runtime_params[p];
    int mt = param_meta.module_type;
    int mi = param_meta.module_index;
    int pi = param_meta.module_param_index;
    if (_runtime_topo.module_params[mt][pi].rate == param_rate::block)
      _plugin_block.block_automation[mt][mi][pi] = _state[mt][mi][pi];
    else
      std::fill(
        _plugin_block.accurate_automation[mt][mi][pi], 
        _plugin_block.accurate_automation[mt][mi][pi] + _host_block.frame_count,
        _state[mt][mi][pi].real);
  }
    
  for (int a = 0; a < _host_block.block_automation.size(); a++)
  {
    auto const& event = _host_block.block_automation[a];
    auto param_meta = _runtime_topo.runtime_params[event.runtime_param_index];
    int mt = param_meta.module_type;
    int mi = param_meta.module_index;
    int pi = param_meta.module_param_index;
    _state[mt][mi][pi] = event.value;
    _plugin_block.block_automation[mt][mi][pi] = event.value;
  }

  std::fill(_accurate_automation_frames.begin(), _accurate_automation_frames.end(), 0);
  for (int a = 0; a < _host_block.accurate_automation.size(); a++)
  {
    auto const& event = _host_block.accurate_automation[a];
    auto rpi = event.runtime_param_index;
    auto param_meta = _runtime_topo.runtime_params[rpi];
    int prev_frame = _accurate_automation_frames[rpi];
    int mt = param_meta.module_type;
    int mi = param_meta.module_index;
    int mpi = param_meta.module_param_index;
    float frame_count = event.frame_index - prev_frame;
    float start = _plugin_block.accurate_automation[mt][mi][mpi][_accurate_automation_frames[rpi]];
    float range = event.value - start;
    for(int f = prev_frame; f < event.frame_index; f++)
      _plugin_block.accurate_automation[mt][mi][mpi][f] = start + (f - prev_frame) / frame_count * range;
    _state[mt][mi][mpi].real = event.value;
    _accurate_automation_frames[rpi] = event.frame_index;
  }

  for(int m = 0; m < _topo.modules.size(); m++)
    for(int i = 0; i < _topo.modules[m].count; i++)
    {
      _plugin_block.module_index = i;
      _topo.modules[i].process(_plugin_block);
    }
}

}