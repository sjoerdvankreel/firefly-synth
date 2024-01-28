#include <iostream>
#include <plugin_base/topo/plugin.hpp>

static int 
fail(std::string const& text)
{
  std::cout << text << std::endl;
  return 1;
}

int 
main(int argc, char** argv)
{
  if(argc != 3) 
    return fail("Usage: param_ref_gen path_to_library path_to_output.html.");
  return 0;
}