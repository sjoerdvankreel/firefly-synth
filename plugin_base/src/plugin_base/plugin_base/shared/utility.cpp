#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/utility.hpp>
#include <juce_core/juce_core.h>

#include <chrono>
#include <cassert>
#include <complex>
#include <fstream>

using namespace juce;

namespace plugin_base {

double
seconds_since_epoch()
{
  auto ticks = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(ticks).count() / 1000000000.0;
}

std::filesystem::path
get_resource_location(format_basic_config const* config)
{
  File file(File::getSpecialLocation(File::currentExecutableFile));
  return config->resources_folder(file.getFullPathName().toStdString());
}

std::vector<char>
file_load(std::filesystem::path const& path)
{
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (stream.bad()) return {};
  std::streamsize size = stream.tellg();
  if (size <= 0) return {};
  stream.seekg(0, std::ios::beg);
  std::vector<char> data(size, 0);
  stream.read(data.data(), size);
  if (stream.bad()) return {};
  return data;
}

cached_fft::
cached_fft(int in_samples):
_in_samples(in_samples),
_output(next_pow2(in_samples) * 2, 0.0f),
_juce_fft(std::log2(next_pow2(in_samples)))
{ assert(in_samples > 1); }

std::vector<float> const& 
cached_fft::perform(std::vector<float> const& in)
{
  _output.clear();
  int np2 = next_pow2(_in_samples);
  assert(in.size() == _in_samples);
  _output.resize(np2 * 2);
  std::copy(in.begin(), in.end(), _output.begin());
  _juce_fft.performRealOnlyForwardTransform(_output.data(), true);
  _output.erase(_output.begin() + np2 / 2, _output.end());

  // scale to 0..1, real part is in even indices, drop imag parts
  float max = std::numeric_limits<float>::min();
  for (int i = 0; i < _output.size(); i += 2)
    max = std::max(max, std::abs(_output[i]));
  for (int i = 0; i < _output.size() / 2; i++)
    _output[i] = max == 0.0f ? 0.0f : std::abs(_output[i * 2]) / max;
  _output.erase(_output.begin() + np2 / 4, _output.end());
  return _output;
}

std::vector<float> 
log_remap_series_x(std::vector<float> const& in, float midpoint)
{
  assert(0 <= midpoint && midpoint <= 1);
  int last_remapped = 0;
  std::vector<float> result(in.size(), 0.0f);
  float exp = std::log(midpoint) / std::log(0.5);
  for (int i = 0; i < in.size(); i++)
  {
    float x = i / (float)in.size();
    int x_remapped = (int)(std::pow(x, exp) * in.size());
    result[x_remapped] = in[i];
    if (x_remapped != last_remapped)
    {
      for(int j = last_remapped + 1; j < x_remapped; j++)
      {
        float w = (j - last_remapped) / (float)(x_remapped - last_remapped);
        result[j] = result[last_remapped] + w * (result[x_remapped] - result[last_remapped]);
      }
      last_remapped = x_remapped;
    }
  }

  return result;
}

}