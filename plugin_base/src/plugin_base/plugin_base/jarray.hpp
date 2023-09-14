#pragma once
#include <plugin_base/utility.hpp>
#include <vector>

namespace plugin_base {

// 2d jagged array
template <class T>
class jarray2d {
  std::vector<std::vector<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray2d);
  void init(std::vector<int> const& dims);

  void clear() { _data.clear(); }
  std::vector<T>& operator[](int i) { return _data[i]; }
  std::vector<T> const& operator[](int i) const { return _data[i]; }
};

// 3d jagged array
template <class T>
class jarray3d {
  std::vector<jarray2d<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray3d);
  void init(std::vector<std::vector<int>> const& dims);

  void clear() { _data.clear(); }
  jarray2d<T>& operator[](int i) { return _data[i]; }
  jarray2d<T> const& operator[](int i) const { return _data[i]; }
};

// 4d jagged array
template <class T>
class jarray4d {
  std::vector<jarray3d<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray4d);
  void init(std::vector<std::vector<std::vector<int>>> const& dims);

  void clear() { _data.clear(); }
  jarray3d<T>& operator[](int i) { return _data[i]; }
  jarray3d<T> const& operator[](int i) const { return _data[i]; }
};

// 5d jagged array
template <class T>
class jarray5d {
  std::vector<jarray4d<T>> _data;
public:
  INF_DECLARE_MOVE_ONLY(jarray5d);
  void init(std::vector<std::vector<std::vector<std::vector<int>>>> const& dims);

  void clear() { _data.clear(); }
  jarray4d<T>& operator[](int i) { return _data[i]; }
  jarray4d<T> const& operator[](int i) const { return _data[i]; }
};

template <class T> void
jarray2d<T>::init(std::vector<int> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].resize(dims[i]);
}

template <class T> void
jarray3d<T>::init(std::vector<std::vector<int>> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].init(dims[i]);
}

template <class T> void
jarray4d<T>::init(std::vector<std::vector<std::vector<int>>> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].init(dims[i]);
}

template <class T> void
jarray5d<T>::init(std::vector<std::vector<std::vector<std::vector<int>>>> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].init(dims[i]);
}

}