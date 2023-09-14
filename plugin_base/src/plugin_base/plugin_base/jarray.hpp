#pragma once
#include <plugin_base/utility.hpp>
#include <vector>

namespace plugin_base {

// 1d jagged array
template <class T>
class jarray1d {
  std::vector<T> _data;
public:
  void resize(int dims);
  INF_DECLARE_MOVE_ONLY(jarray1d);
  explicit jarray1d(std::size_t size, T const& val): _data(size, val) {}

  decltype(_data.end()) end() { return _data.end(); }
  decltype(_data.begin()) begin() { return _data.begin(); }

  T& operator[](int i) { return _data[i]; }
  T const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void emplace_back() { _data.emplace_back(); }
  void push_back(T const& val) { _data.push_back(val); }
};

// 2d jagged array
template <class T>
class jarray2d {
  std::vector<jarray1d<T>> _data;
public:
  void resize(jarray1d<int> const& dims);
  INF_DECLARE_MOVE_ONLY(jarray2d);
  explicit jarray2d(std::size_t size, jarray1d<T> const& val) : _data(size, val) {}

  jarray1d<T>& operator[](int i) { return _data[i]; }
  jarray1d<T> const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void push_back(jarray1d<T> const& val) { _data.push_back(val); }
  template <class... U> void emplace_back(U&&... args) { _data.emplace_back(std::forward<U>(args)...); }
};

// 3d jagged array
template <class T>
class jarray3d {
  std::vector<jarray2d<T>> _data;
public:
  void resize(jarray2d<int> const& dims);
  INF_DECLARE_MOVE_ONLY(jarray3d);
  explicit jarray3d(std::size_t size, jarray2d<T> const& val) : _data(size, val) {}

  jarray2d<T>& operator[](int i) { return _data[i]; }
  jarray2d<T> const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void emplace_back() { _data.emplace_back(); }
  void push_back(jarray2d<T> const& val) { _data.push_back(val); }
  template <class... U> void emplace_back(U&&... args) { _data.emplace_back(std::forward<U>(args)...); }
};

// 4d jagged array
template <class T>
class jarray4d {
  std::vector<jarray3d<T>> _data;
public:
  void resize(jarray3d<int> const& dims);
  INF_DECLARE_MOVE_ONLY(jarray4d);
  explicit jarray4d(std::size_t size, jarray3d<T> const& val) : _data(size, val) {}

  jarray3d<T>& operator[](int i) { return _data[i]; }
  jarray3d<T> const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void emplace_back() { _data.emplace_back(); }
  void push_back(jarray3d<T> const& val) { _data.push_back(val); }
  template <class... U> void emplace_back(U&&... args) { _data.emplace_back(std::forward<U>(args)...); }
};

// 5d jagged array
template <class T>
class jarray5d {
  std::vector<jarray4d<T>> _data;
public:
  void resize(jarray4d<int> const& dims);
  INF_DECLARE_MOVE_ONLY(jarray5d);
  explicit jarray5d(std::size_t size, jarray4d<T> const& val) : _data(size, val) {}

  jarray4d<T>& operator[](int i) { return _data[i]; }
  jarray4d<T> const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void emplace_back() { _data.emplace_back(); }
  void push_back(jarray4d<T> const& val) { _data.push_back(val); }
  template <class... U> void emplace_back(U&&... args) { _data.emplace_back(std::forward<U>(args)...); }
};

template <class T> void
jarray1d<T>::resize(int dims)
{
  _data.resize(dims);
}

template <class T> void
jarray2d<T>::resize(jarray1d<int> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].resize(dims[i]);
}

template <class T> void
jarray3d<T>::resize(jarray2d<int> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].resize(dims[i]);
}

template <class T> void
jarray4d<T>::resize(jarray3d<int> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].resize(dims[i]);
}

template <class T> void
jarray5d<T>::resize(jarray4d<int> const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].resize(dims[i]);
}

}