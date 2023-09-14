#pragma once
#include <plugin_base/utility.hpp>
#include <vector>

namespace plugin_base {

template <class T, int Dims>
class jarray;

template <class T>
class jarray<T, 1> {
  std::vector<T> _data;
public:
  void resize(int dims);
  INF_DECLARE_MOVE_ONLY(jarray);
  explicit jarray(std::size_t size, T const& val) : _data(size, val) {}

  decltype(_data.end()) end() { return _data.end(); }
  decltype(_data.begin()) begin() { return _data.begin(); }

  T& operator[](int i) { return _data[i]; }
  T const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void emplace_back(T&& val) { _data.emplace_back(val); }
  void push_back(T const& val) { _data.push_back(val); }
};

template <class T, int Dims>
class jarray {
  std::vector<jarray<T, Dims - 1>> _data;
public:
  void resize(jarray<int, Dims - 1> const& dims);
  INF_DECLARE_MOVE_ONLY(jarray);
  explicit jarray(std::size_t size, jarray<T, Dims - 1> const& val) : _data(size, val) {}

  jarray<T, Dims - 1>& operator[](int i) { return _data[i]; }
  jarray<T, Dims - 1> const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void push_back(jarray<T, Dims - 1> const& val) { _data.push_back(val); }
  template <class... U> void emplace_back(U&&... args) { _data.emplace_back(std::forward<U>(args)...); }
};

template <class T> void
jarray<T, 1>::resize(int dims)
{
  _data.resize(dims);
}

template <class T, int Dims> void
jarray<T, Dims>::resize(jarray<int, Dims - 1> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].resize(dims[i]);
}

template <class T> using jarray1d = jarray<T, 1>;
template <class T> using jarray2d = jarray<T, 2>;
template <class T> using jarray3d = jarray<T, 3>;
template <class T> using jarray4d = jarray<T, 4>;
template <class T> using jarray5d = jarray<T, 5>;


}