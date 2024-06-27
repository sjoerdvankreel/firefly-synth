#pragma once

#include <string>

#define PB_WRITE_LOG(msg) ::plugin_base::write_log(__FILE__, __LINE__, __func__, msg)

namespace plugin_base {

void cleanup_logging();
void init_logging(std::string const& vendor, std::string const& full_name);

void write_log(std::string const& file, int line, std::string const& func, std::string const& message);

}