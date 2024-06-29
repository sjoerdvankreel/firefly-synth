#pragma once

#include <string>

#define PB_WRITE_LOG(msg) ::plugin_base::write_log(__FILE__, __LINE__, __func__, msg)
#define PB_LOG_FUNC_ENTRY_EXIT() func_entry_exit_logger \
PB_COMBINE(func_entry_exit_logger_, __LINE__)(__FILE__, __LINE__, __func__)

namespace plugin_base {

void cleanup_logging();
void init_logging(std::string const& vendor, std::string const& full_name);
void write_log(std::string const& file, int line, std::string const& func, std::string const& message);

class func_entry_exit_logger {
  char const* const _file;
  int const _line;
  char const* const _func;
public:
  ~func_entry_exit_logger() { write_log(_file, _line, _func, "Function exit."); }
  func_entry_exit_logger(char const* file, int line, char const* func) :
  _file(file), _line(line), _func(func) { write_log(_file, _line, _func, "Function enter."); }
};

}