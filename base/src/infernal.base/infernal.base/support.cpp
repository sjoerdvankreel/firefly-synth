#include <infernal.base/support.hpp>

namespace infernal::base {

std::vector<item_topo>
note_name_items() 
{
  std::vector<item_topo> result;
  result.emplace_back("{BAE982FB-29FA-4E6A-B8E9-88DED2B71F97}", "C");
  result.emplace_back("{F6B7C5C1-2F00-48C3-BB2B-95F6D26C4371}", "C#");
  result.emplace_back("{80E1A69D-F1D6-4D34-A7AC-762014D3D547}", "D");
  result.emplace_back("{10D8A2F6-AECE-4089-A76F-3187756489BC}", "D#");
  result.emplace_back("{98AA08E3-F29E-41F9-AB9A-7FD9CB4C2408}", "E");
  result.emplace_back("{3B362152-0D3D-4669-91E2-0C375859C4EC}", "F");
  result.emplace_back("{F063ACFF-B00F-496C-8B29-1CA6E85B3A37}", "F#");
  result.emplace_back("{1910726E-A4A6-4430-85E4-3355340A310D}", "G");
  result.emplace_back("{9136132A-5C63-43FE-8218-D9B17C53F82E}", "G#");
  result.emplace_back("{C4E5CD80-527D-4AA8-A1CD-AAC3A5165AB5}", "A");
  result.emplace_back("{AC865BAA-5AF9-4C62-883F-140756310CC6}", "A#");
  result.emplace_back("{79231D69-2FF6-4A49-B30A-54281EA696D9}", "B");
  return result;
};

}