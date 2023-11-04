#pragma once

#include <plugin_base/shared/utility.hpp>
#include <vector>

namespace plugin_base {

// jagged array

template <class T, int Dims>
class jarray;
template <class T, int Dims>
struct jarray_traits;

template <class T>
struct jarray_traits<T, 1> final {
  typedef T elem_type;
  typedef int dims_type;
  static void fill(std::vector<elem_type>& v, elem_type value)
  { std::fill(v.begin(), v.end(), value); }
  static void resize(std::vector<elem_type>& v, dims_type const& dims)
  { v.resize(dims); }
};

template <class T, int Dims>
struct jarray_traits final {
  typedef jarray<T, Dims - 1> elem_type;
  typedef jarray<int, Dims - 1> dims_type;
  static void fill(std::vector<elem_type>& v, T value);
  static void resize(std::vector<elem_type>& v, dims_type const& dims);
};

template <class T, int Dims>
class jarray final {
  typedef typename jarray_traits<T, Dims>::dims_type dims_type;
  typedef typename jarray_traits<T, Dims>::elem_type elem_type;
  std::vector<elem_type> _data;

public:
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(jarray);
  explicit jarray(std::size_t size, elem_type const& val) :
  _data(size, val) {}

  void fill(T value) 
  { jarray_traits<T, Dims>::fill(_data, value); }
  void resize(dims_type const& dims) 
  { jarray_traits<T, Dims>::resize(_data, dims); }
  elem_type& operator[](int i) { return _data[i]; }
  elem_type const& operator[](int i) const { return _data[i]; }

  decltype(_data.end()) end() { return _data.end(); }
  decltype(_data.begin()) begin() { return _data.begin(); }
  decltype(_data.end()) end() const { return _data.end(); }
  decltype(_data.begin()) begin() const { return _data.begin(); }
  decltype(_data.cend()) cend() const { return _data.cend(); }
  decltype(_data.cbegin()) cbegin() const { return _data.cbegin(); }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void push_back(elem_type const& val) { _data.push_back(val); }

  template <class... U> decltype(auto) emplace_back(U&&... args) 
  { return _data.emplace_back(std::forward<U>(args)...); }
  void fill(int start, int end, elem_type const& val)
  { std::fill(begin() + start, begin() + end, val); }
  void add_to(int start, int end, jarray& rhs) const
  { for(int f = start; f < end; f++) rhs[f] += (*this)[f]; }
  void copy_to(int start, int end, jarray& rhs) const
  { std::copy(cbegin() + start, cbegin() + end, rhs.begin() + start); }
  template <class Transform> void transform(int start, int end, Transform transform)
  { std::transform(cbegin() + start, cbegin() + end, begin() + start, transform); }
  template <class Transform> void transform_to(int start, int end, jarray& rhs, Transform transform) const
  { std::transform(cbegin() + start, cbegin() + end, rhs.begin() + start, transform); }
};

template <class T, int Dims>
void jarray_traits<T, Dims>::fill(std::vector<elem_type>& v, T value)
{
  for (int i = 0; i < v.size(); i++)
    v[i].fill(value);
}

template <class T, int Dims>
void jarray_traits<T, Dims>::resize(std::vector<elem_type>& v, dims_type const& dims)
{
  v.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    v[i].resize(dims[i]);
}

}