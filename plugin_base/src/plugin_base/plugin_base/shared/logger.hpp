#pragma once

#include <string>

namespace plugin_base {

void cleanup_logging();
void init_logging(std::string const& vendor, std::string const& full_name);

void write_log(std::string const& message);

}