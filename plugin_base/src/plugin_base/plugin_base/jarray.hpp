#pragma once
#include <plugin_base/utility.hpp>
#include <vector>

namespace infernal::base {

// 2d jagged array
template <class T>
class jarray2d {
  std::vector<std::vector<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray2d);
  void clear() { _data.clear(); }
  std::vector<T>& operator[](int i) { return _data[i]; }
  std::vector<T> const& operator[](int i) const { return _data[i]; }

  void 
  init(std::vector<int> const& dims)
  {
    for (int i = 0; i < dims.size(); i++)
    {
      _data.emplace_back();
      for (int j = 0; j < dims[i]; j++)
        _data[i].emplace_back();
    }
  }
};

// 3d jagged array
template <class T>
class jarray3d {
  std::vector<jarray2d<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray3d);
  void clear() { _data.clear(); }
  jarray2d<T>& operator[](int i) { return _data[i]; }
  jarray2d<T> const& operator[](int i) const { return _data[i]; }

  void 
  init(std::vector<std::vector<int>> const& dims)
  {
    for (int i = 0; i < dims.size(); i++)
    {
      _data.emplace_back();
      _data[i].init(dims[i]);
    }
  }
};

// 4d jagged array
template <class T>
class jarray4d {
  std::vector<jarray3d<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray4d);
  void clear() { _data.clear(); }
  jarray3d<T>& operator[](int i) { return _data[i]; }
  jarray3d<T> const& operator[](int i) const { return _data[i]; }

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