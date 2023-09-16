#include <plugin_base/io.hpp>
#include <juce_core/juce_core.h>

#include <memory>
#include <fstream>

using namespace juce;

namespace plugin_base {

static int const format_version = 1;
static std::string const format_magic = "{296BBDE2-6411-4A85-BFAF-A9A7B9703DF0}";

bool
plugin_io::save_file(std::filesystem::path const& path, jarray<plain_value, 4> const& state) const
{
  std::ofstream stream(path, std::ios::out | std::ios::binary);
  if (stream.bad()) return false;
  auto data(save(state));
  stream.write(data.data(), data.size());
  return !stream.bad();
}

bool
plugin_io::load_file(std::filesystem::path const& path, jarray<plain_value, 4>& state) const
{
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if(stream.bad()) return false;
  std::streamsize size = stream.tellg();
  stream.seekg(0, std::ios::beg);
  std::vector<char> data(size, 0);
  stream.read(data.data(), size);
  if (stream.bad()) return false;
  return load(data, state);
}

std::vector<char>
plugin_io::save(jarray<plain_value, 4> const& state) const
{
  auto root = std::make_unique<DynamicObject>();
  auto format = std::make_unique<DynamicObject>();
  format->setProperty("magic", var(format_magic));
  format->setProperty("version", var(format_version));
  root->setProperty("format", var(format.release()));
  std::string json = JSON::toString(var(root.release())).toStdString();
  return std::vector<char>(json.begin(), json.end());
}

bool 
plugin_io::load(std::vector<char> const& data, jarray<plain_value, 4>& state) const
{
  var root;
  std::string json(data.size(), '\0');
  std::copy(data.begin(), data.end(), json.begin());
  auto result = JSON::parse(String(json), root);
  if(!result.wasOk()) return false;
  var format = root["format"];
  if(format["magic"] != String(format_magic)) return false;
  if((int)format["version"] != format_version) return false;
  return true;
}

}