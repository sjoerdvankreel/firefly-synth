#include <firefly_synth/waves.hpp>
#include <plugin_base/topo/support.hpp>

using namespace plugin_base;

// waveforms for lfos and shapers
namespace firefly_synth
{

static std::string
wave_make_header_skew(int skew)
{
  switch (skew)
  {
  case wave_skew_type_off: return "Off";
  case wave_skew_type_lin: return "Linear";
  case wave_skew_type_scu: return "ScaleUni";
  case wave_skew_type_scb: return "ScaleBi";
  case wave_skew_type_xpu: return "ExpUni";
  case wave_skew_type_xpb: return "ExpBi";
  default: assert(false); return {};
  }
}

static std::string
wave_make_name_skew(int skew)
{
  switch (skew)
  {
  case wave_skew_type_off: return "Of";
  case wave_skew_type_lin: return "Ln";
  case wave_skew_type_scu: return "Su";
  case wave_skew_type_scb: return "Sb";
  case wave_skew_type_xpu: return "Xu";
  case wave_skew_type_xpb: return "Xb";
  default: assert(false); return {};
  }
}

static std::string
wave_make_header_shape(int shape)
{
  switch (shape)
  {
  case wave_shape_type_saw: return "Saw";
  case wave_shape_type_sqr: return "Square";
  case wave_shape_type_sin: return "Sine";
  default: assert(false); return {};
  }
}

static std::string
wave_make_name_shape(int shape)
{
  switch (shape)
  {
  case wave_shape_type_saw: return "Saw";
  case wave_shape_type_sqr: return "Sqr";
  case wave_shape_type_sin: return "Sin";
  default: assert(false); return {};
  }
}

std::string 
wave_make_header_shape_x(int shape, int skew_x)
{
  auto shape_header = wave_make_header_shape(shape);
  auto x_header = wave_make_header_skew(skew_x);
  return shape_header + "." + x_header + "X";
}

std::string 
wave_make_name(int shape, int skew_x, int skew_y)
{
  auto name_x = wave_make_name_skew(skew_x);
  auto name_y = wave_make_name_skew(skew_y);
  auto name_shape = wave_make_name_shape(shape);
  return name_shape + "." + name_x + "X/" + name_y + "Y";
}

std::vector<topo_tag> 
wave_skew_type_tags()
{
  std::vector<topo_tag> result;
  result.push_back(make_topo_tag("{B15C7C6E-B1A4-49D3-85EF-12A7DC9EAA83}", wave_make_name_skew(wave_skew_type_off)));
  result.push_back(make_topo_tag("{431D0E01-096B-4229-9ACE-25EFF7F2D4F0}", wave_make_name_skew(wave_skew_type_lin)));
  result.push_back(make_topo_tag("{106A1510-3B99-4CC8-88D4-6D82C117EC33}", wave_make_name_skew(wave_skew_type_scu)));
  result.push_back(make_topo_tag("{905936B8-3083-4293-A549-89F3979E02B7}", wave_make_name_skew(wave_skew_type_scb)));
  result.push_back(make_topo_tag("{606B62CB-1C17-42CA-931B-61FA4C22A9F0}", wave_make_name_skew(wave_skew_type_xpu)));
  result.push_back(make_topo_tag("{66CE54E3-84A7-4279-BF93-F0367266B389}", wave_make_name_skew(wave_skew_type_xpb)));
  return result;
}

std::vector<topo_tag> 
wave_shape_type_tags()
{
  std::vector<topo_tag> result;
  result.push_back(make_topo_tag("{CA30E83B-2A11-4833-8A45-81F666A3A4F5}", wave_make_name_shape(wave_shape_type_saw)));
  result.push_back(make_topo_tag("{7176FE9E-D2A8-44FE-B312-93D712173D29}", wave_make_name_shape(wave_shape_type_sqr)));
  result.push_back(make_topo_tag("{4A873C32-8B89-47ED-8C93-44FE0B6A7DCC}", wave_make_name_shape(wave_shape_type_sin)));
  return result;
}

multi_menu 
make_wave_multi_menu()
{
  return make_multi_menu(
    wave_shape_type_tags(), wave_skew_type_tags(), wave_skew_type_tags(),
    wave_make_header_shape, wave_make_header_shape_x, wave_make_name);
}

}