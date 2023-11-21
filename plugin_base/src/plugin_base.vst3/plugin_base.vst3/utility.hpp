#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/state.hpp>

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>

#include <vector>

namespace plugin_base::vst3 {

Steinberg::FUID fuid_from_text(char const* text);
std::vector<char> load_ibstream(Steinberg::IBStream* stream);

struct vst3_config:
public format_config
{
  static inline vst3_config const* instance() 
  { static vst3_config result; return &result; }
  std::filesystem::path resources_folder(std::filesystem::path const& binary_path) const override
  { return binary_path.parent_path().parent_path() / "Resources"; }
};

}