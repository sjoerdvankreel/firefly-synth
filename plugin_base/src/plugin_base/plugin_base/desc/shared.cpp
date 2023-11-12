#include <plugin_base/desc/shared.hpp>

namespace plugin_base {

std::string
desc_id(topo_info const& info, int slot)
{
  std::string result = info.tag.id;
  result += "-" + std::to_string(slot);
  return result;
}

std::string
desc_name(topo_info const& info, int slot)
{
  std::string result = info.tag.name;
  if (info.slot_count > 1) result += " " + std::to_string(slot + (info.tag.name_one_based? 1: 0));
  return result;
}

void
desc_info::validate(int topo_count) const
{
  assert(id.size());
  assert(name.size());
  assert(global >= 0);
  assert(id_hash >= 0);
  assert(0 <= topo && topo < topo_count);
}

void
topo_desc_info::validate(int topo_count, int slot_count) const
{
  desc_info::validate(topo_count);
  assert(0 <= slot && slot < slot_count);
}

int
desc_id_hash(std::string const& text)
{
  // stable hash, nonnegative required for vst3 param tags
  std::uint32_t h = 0;
  int const multiplier = 33;
  auto utext = reinterpret_cast<std::uint8_t const*>(text.c_str());
  for (auto const* p = utext; *p != '\0'; p++) h = multiplier * h + *p;
  return std::abs(static_cast<int>(h + (h >> 5)));
}

}
