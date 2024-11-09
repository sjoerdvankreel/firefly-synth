#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/utility.hpp>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

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

std::vector<float> 
fft(std::vector<float> const& in)
{
  std::size_t pow2 = next_pow2(in.size());
  std::vector<float> out(pow2, 0.0f);
  std::copy(in.begin(), in.end(), out.begin());
  std::size_t order = std::log2(pow2);
  juce::dsp::FFT fft(order);
  fft.performRealOnlyForwardTransform(out.data(), true);
  out.erase(out.begin() + pow2 / 2, out.end());
  
  // scale to 0..1
  float max = std::numeric_limits<float>::min();
  for (int i = 0; i < out.size(); i++)
    max = std::max(max, std::abs(out[i]));
  for (int i = 0; i < out.size(); i++)
    out[i] = max == 0.0f ? 0.0f : std::abs(out[i]) / max;
  return out;
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