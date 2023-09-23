#pragma once
#if (defined __linux__) || (defined  __FreeBSD__)

namespace plugin_base::clap {
int current_display_fd();
}

#endif