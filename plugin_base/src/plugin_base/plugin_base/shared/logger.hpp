#pragma once

#include <string>

#define PB_WRITE_LOG(msg) ::plugin_base::write_log(__FILE__, __LINE__, __func__, msg)
#define PB_WRITE_LOG_FUNC_EXIT() PB_WRITE_LOG("Exit function.")
#define PB_WRITE_LOG_FUNC_ENTER() PB_WRITE_LOG("Entering function...")

namespace plugin_base {

void cleanup_logging();
void init_logging(std::string const& vendor, std::string const& full_name);

void write_log(std::string const& file, int line, std::string const& func, std::string const& message);

}