#define _CRT_SECURE_NO_WARNINGS 1

#include <plugin_base/shared/logger.hpp>
#include <plugin_base/shared/io_shared.hpp>

#include <juce_core/juce_core.h>

#include <memory>
#include <chrono>
#include <filesystem>

namespace plugin_base {

static int constexpr max_log_size = 1024 * 1024;
static std::unique_ptr<juce::Uuid> _instance_id = {};
static std::unique_ptr<juce::FileLogger> _logger = {};

void 
cleanup_logging()
{
  _instance_id.reset();
  _logger.reset();
}

void 
init_logging(std::string const& vendor, std::string const& full_name)
{
  // TODO make it work on linux
  auto user_data = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
  auto user_data_str = user_data.getFullPathName().toStdString();
  auto path = std::filesystem::path(user_data_str) / user_location(vendor, full_name) / "plugin.log";
  auto file = juce::File(juce::String(path.string()));
  _logger = std::make_unique<juce::FileLogger>(file, juce::String(full_name), max_log_size);
  _instance_id = std::make_unique<juce::Uuid>();
}

void 
write_log(std::string const& file, int line, std::string const& func, std::string const& message)
{
  // allow unitialized usage for reference generator
  if (!_instance_id) return;

  std::filesystem::path path(file);
  auto file_name = path.filename().string();
  
  // gcc doesnt support std::format yet
  time_t rawtime;
  struct tm* timeinfo;
  char buffer[80];

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
  std::string date_time(buffer);

  std::string instance_id = _instance_id->toString().toStdString();
  std::string full_message = date_time + ": " + "instance " + instance_id + ": " + file_name + " line " + std::to_string(line) + ", " + func + ": " + message;
  _logger->logMessage(full_message);
}

}
