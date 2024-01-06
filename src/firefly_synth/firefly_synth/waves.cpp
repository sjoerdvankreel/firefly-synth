#include <firefly_synth/waves.hpp>

namespace firefly_synth
{

std::string 
wave_id(int shape, int skew_x, int skew_y)
{
  std::string skew_off_id = "{DAE445A3-2A8A-41D1-8F9B-4F0B75146E20}";
  std::string skew_lin_id = "{C8102C86-7FCC-44AD-8D08-54C5391D4A26}";
  std::string skew_pow_id = "{E54FCE99-9F39-494F-BC9F-75923604B904}";

  std::string shape_saw_id = "{DC3FD52F-4FF8-470D-9949-5DCDC5D99437}";
  std::string shape_sqr_id = "{7BF99D04-6400-4ED1-BB73-ABD8913C50D2}";
  std::string shape_sin_id = "{57581B51-427F-49D2-A68A-A811F4DDD9AD}";

  std::vector<std::string> skew_ids = { skew_off_id, skew_lin_id, skew_pow_id };
  std::vector<std::string> shape_ids = { shape_saw_id, shape_sqr_id, shape_sin_id };
  return shape_ids[shape] + "-" + skew_ids[skew_x] + "-" + skew_ids[skew_y];
}

std::string
wave_name(int shape, int skew_x, int skew_y)
{
  std::string skew_off_name = "{DAE445A3-2A8A-41D1-8F9B-4F0B75146E20}";
  std::string skew_lin_name = "{C8102C86-7FCC-44AD-8D08-54C5391D4A26}";
  std::string skew_pow_name = "{E54FCE99-9F39-494F-BC9F-75923604B904}";

  std::string shape_saw_name = "{DC3FD52F-4FF8-470D-9949-5DCDC5D99437}";
  std::string shape_sqr_name = "{7BF99D04-6400-4ED1-BB73-ABD8913C50D2}";
  std::string shape_sin_name = "{57581B51-427F-49D2-A68A-A811F4DDD9AD}";

  std::vector<std::string> skew_names = { skew_off_name, skew_lin_name, skew_pow_name };
  std::vector<std::string> shape_names = { shape_saw_name, shape_sqr_name, shape_sin_name };
  return shape_names[shape] + ".X" + skew_names[skew_x] + "/Y" + skew_names[skew_y];
}

}
