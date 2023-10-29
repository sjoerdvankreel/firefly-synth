#include <plugin_base/shared/utility.hpp>

#include <chrono>
#include <fstream>

namespace plugin_base {

double
seconds_since_epoch()
{
  auto ticks = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(ticks).count() / 1000000000.0;
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

}