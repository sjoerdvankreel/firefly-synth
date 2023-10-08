#include <plugin_base/shared/utility.hpp>
#include <chrono>

namespace plugin_base {

std::string 
join_string(std::vector<std::string> const& strings, std::string const& delim)
{
  if(strings.empty()) return std::string();
  std::string result = strings[0];
  for(int i = 1; i < strings.size(); i++)
    result += delim + strings[i];
  return result;
}

double
seconds_since_epoch()
{
  auto ticks = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(ticks).count() / 1000000000.0;
}

}