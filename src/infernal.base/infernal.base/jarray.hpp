#pragma once
#include <infernal.base/utility.hpp>
#include <vector>
#include <cassert>

namespace infernal::base {

template <class T>
class jarray3d {
  std::vector<std::vector<std::vector<T>>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray3d);
  void clear() { _data.clear(); }

  T* data(int dim0, int dim1) 
  { return _data[dim0][dim1].data(); }
  T const* data(int dim0, int dim1) const 
  { return _data[dim0][dim1].data(); }

  void init(
    int dim0, 
    std::vector<int> const& dim1, 
    std::vector<std::vector<int>> const& dim2)
  {
    for (int i0 = 0; i0 < dim0; i0++)
    {
      _data.emplace_back();
      for (int i1 = 0; i1 < dim1[i0]; i1++)
      {
        _data[i0].emplace_back();
        for(int i2 = 0; i2 < dim2[i0][i1]; i2++)
          _data[i0][i1].emplace_back();
      }
    }
  }
};

template <class T>
class jarray4d {
  std::vector<jarray3d<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray4d);
  void clear() { _data.clear(); }

  T* data(int dim0, int dim1, int dim2) 
  { return _data[dim0].data(dim1, dim2); }
  T const* data(int dim0, int dim1, int dim2) const 
  { return _data[dim0].data(dim1, dim2); }

  void init(
    int dim0, 
    std::vector<int> const& dim1, 
    std::vector<std::vector<int>> const& dim2,
    std::vector<std::vector<std::vector<int>>> const& dim3)
  {
    for (int i0 = 0; i0 < dim0; i0++)
    {
      _data.emplace_back();
      _data[i0].init(dim1[i0], dim2[i0], dim3[i0]);
    }
  }
};

}