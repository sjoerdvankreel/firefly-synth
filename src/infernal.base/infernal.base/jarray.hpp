#pragma once
#include <infernal.base/utility.hpp>
#include <vector>

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

  void 
  init(std::vector<std::vector<int>> const& dims)
  {
    for (int i = 0; i < dims.size(); i++)
    {
      _data.emplace_back();
      for (int j = 0; j < dims[i].size(); j++)
      {
        _data[j].emplace_back();
        for(int k = 0; k < dims[i][j]; k++)
          _data[i][j].emplace_back();
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

  void 
  init(std::vector<std::vector<std::vector<int>>> const& dims)
  {
    for (int i = 0; i < dims.size(); i++)
    {
      _data.emplace_back();
      _data[i].init(dims[i]);
    }
  }
};

}