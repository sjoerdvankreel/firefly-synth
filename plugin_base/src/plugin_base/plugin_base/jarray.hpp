#pragma once
#include <plugin_base/utility.hpp>
#include <vector>

namespace plugin_base {

template <class T, int Dims>
class jarray;
template <class T, int Dims>
struct jarray_traits;

template <class T>
struct jarray_traits<T, 1> {
  typedef int dims_type;
  typedef T element_type;
};

template <class T, int Dims>
struct jarray_traits {
  typedef jarray<int, Dims - 1> dims_type;
  typedef jarray<T, Dims - 1> element_type;
};

template <class T>
class jarray<T, 1> {
  std::vector<typename jarray_traits<T, 1>::element_type> _data;
public:
  void resize(typename jarray_traits<T, 1>::dims_type const& dims);
  INF_DECLARE_MOVE_ONLY(jarray);
  explicit jarray(std::size_t size, typename jarray_traits<T, 1>::element_type const& val) : _data(size, val) {}

  decltype(_data.end()) end() { return _data.end(); }
  decltype(_data.begin()) begin() { return _data.begin(); }

  typename jarray_traits<T, 1>::element_type& operator[](int i) { return _data[i]; }
  typename jarray_traits<T, 1>::element_type const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void push_back(typename jarray_traits<T, 1>::element_type const& val) { _data.push_back(val); }
  template <class... U> decltype(auto) emplace_back(U&&... args) { _data.emplace_back(std::forward<U>(args)...); }
};

template <class T, int Dims>
class jarray {
  std::vector<typename jarray_traits<T, Dims>::element_type> _data;
public:
  void resize(typename jarray_traits<T, Dims>::dims_type const& dims);
  INF_DECLARE_MOVE_ONLY(jarray);
  explicit jarray(std::size_t size, typename jarray_traits<T, Dims>::element_type const& val) : _data(size, val) {}

  decltype(_data.end()) end() { return _data.end(); }
  decltype(_data.begin()) begin() { return _data.begin(); }

  typename jarray_traits<T, Dims>::element_type& operator[](int i) { return _data[i]; }
  typename jarray_traits<T, Dims>::element_type const& operator[](int i) const { return _data[i]; }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void push_back(typename jarray_traits<T, Dims>::element_type const& val) { _data.push_back(val); }
  template <class... U> decltype(auto) emplace_back(U&&... args) { _data.emplace_back(std::forward<U>(args)...); }
};

template <class T> void
jarray<T, 1>::resize(typename jarray_traits<T, 1>::dims_type const& dims)
{
  _data.resize(dims);
}

template <class T, int Dims> void
jarray<T, Dims>::resize(typename jarray_traits<T, Dims>::dims_type const& dims)
{
  _data.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    _data[i].resize(dims[i]);
}

}