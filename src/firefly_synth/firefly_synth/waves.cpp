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
  case wave_shape_type_sqr: return "Sqr";
  case wave_shape_type_tri: return "Tri";
  case wave_shape_type_sin: return "Sin";
  case wave_shape_type_cos: return "Cos";
  case wave_shape_type_sin_sin: return "SinSin";
  case wave_shape_type_sin_cos: return "SinCos";
  case wave_shape_type_cos_sin: return "CosSin";
  case wave_shape_type_cos_cos: return "CosCos";
  case wave_shape_type_sin_sin_sin: return "SinSinSin";
  case wave_shape_type_sin_sin_cos: return "SinSinCos";
  case wave_shape_type_sin_cos_sin: return "SinCosSin";
  case wave_shape_type_sin_cos_cos: return "SinCosCos";
  case wave_shape_type_cos_sin_sin: return "CosSinSin";
  case wave_shape_type_cos_sin_cos: return "CosSinCos";
  case wave_shape_type_cos_cos_sin: return "CosCosSin";
  case wave_shape_type_cos_cos_cos: return "CosCosCos";
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
  case wave_shape_type_tri: return "Tri";
  case wave_shape_type_sin: return "Sin";
  case wave_shape_type_cos: return "Cos";
  case wave_shape_type_sin_sin: return "SnSn";
  case wave_shape_type_sin_cos: return "SnCs";
  case wave_shape_type_cos_sin: return "CsSn";
  case wave_shape_type_cos_cos: return "CsCs";
  case wave_shape_type_sin_sin_sin: return "SSS";
  case wave_shape_type_sin_sin_cos: return "SSC";
  case wave_shape_type_sin_cos_sin: return "SCS";
  case wave_shape_type_sin_cos_cos: return "SCC";
  case wave_shape_type_cos_sin_sin: return "CSS";
  case wave_shape_type_cos_sin_cos: return "CSC";
  case wave_shape_type_cos_cos_sin: return "CCS";
  case wave_shape_type_cos_cos_cos: return "CCC";
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
  result.push_back(make_topo_tag("{41D6859E-3A16-432A-8851-D4E5D3F39662}", wave_make_name_shape(wave_shape_type_tri)));
  result.push_back(make_topo_tag("{4A873C32-8B89-47ED-8C93-44FE0B6A7DCC}", wave_make_name_shape(wave_shape_type_sin)));
  result.push_back(make_topo_tag("{102A7369-1994-41B1-9E2E-EC96AB60162E}", wave_make_name_shape(wave_shape_type_cos)));
  result.push_back(make_topo_tag("{B6B07567-00C8-4076-B60F-D2AC10CE935A}", wave_make_name_shape(wave_shape_type_sin_sin)));
  result.push_back(make_topo_tag("{B1305EE8-57EF-4BC6-8F3A-7A8BBD2359F2}", wave_make_name_shape(wave_shape_type_sin_cos)));
  result.push_back(make_topo_tag("{FA227C0D-C604-45B3-B5DF-0E6C46FD9C2F}", wave_make_name_shape(wave_shape_type_cos_sin)));
  result.push_back(make_topo_tag("{37D39A3C-2058-4DC6-A9AE-DFBB423EB0D2}", wave_make_name_shape(wave_shape_type_cos_cos)));
  result.push_back(make_topo_tag("{CE36CD8E-5D1F-40F0-85E3-5DAD99AFC53E}", wave_make_name_shape(wave_shape_type_sin_sin_sin)));
  result.push_back(make_topo_tag("{D921DA52-4D30-4AB7-95F3-CAA65C4F83AA}", wave_make_name_shape(wave_shape_type_sin_sin_cos)));
  result.push_back(make_topo_tag("{C8D7BA33-6458-4972-8C31-D9BDAE0A3A54}", wave_make_name_shape(wave_shape_type_sin_cos_sin)));
  result.push_back(make_topo_tag("{F67FB33F-CEDF-4F43-AD23-356775EECED2}", wave_make_name_shape(wave_shape_type_sin_cos_cos)));
  result.push_back(make_topo_tag("{84E3B508-2AAA-4EBA-AD8C-B5AD1A055342}", wave_make_name_shape(wave_shape_type_cos_sin_sin)));
  result.push_back(make_topo_tag("{B191D364-1951-449A-ABC7-09AEE9DB9FC4}", wave_make_name_shape(wave_shape_type_cos_sin_cos)));
  result.push_back(make_topo_tag("{094482D1-5BAC-4F70-80F3-CA3924DDFBE6}", wave_make_name_shape(wave_shape_type_cos_cos_sin)));
  result.push_back(make_topo_tag("{6A56691C-0F9C-4CE1-B835-85CF4D3B1F9B}", wave_make_name_shape(wave_shape_type_cos_cos_cos)));
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