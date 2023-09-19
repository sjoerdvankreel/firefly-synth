#pragma once

#include <plugin_base/utility.hpp>

#include <array>
#include <vector>

namespace plugin_base {

// multidimensional array

template <int Dims>
struct marray_dims
{ 
  int dims[Dims]; 
  int index(marray_dims<Dims> indices) const;
};

template <class T, int Dims>
class marray final {
  std::vector<T> _data = {};
  marray_dims<Dims> _dims = {};

public:
  INF_DECLARE_MOVE_ONLY(marray);

  explicit marray(marray_dims<Dims> dims);
  template <class... D> explicit marray(D&&... dims): 
  marray(marray_dims<Dims> { std::forward<D>(dims)... }) 
  { static_assert(sizeof...(dims) == Dims); }

  template <class... I> T& operator()(I&&... indices);
  template <class... I> T const& operator()(I&&... indices) const;
  T& operator()(marray_dims<Dims> indices) { return _data[_dims.index(indices)]; }
  T const& operator()(marray_dims<Dims> indices) const { return _data[_dims.index(indices)]; }
};

template <class T, int Dims>
marray<T, Dims>::marray(marray_dims<Dims> dims) : 
_dims(dims)
{
  int size = dims.dims[0];
  for(int i = 1; i < Dims; i++)
    size *= dims.dims[i];
  _data.resize(size, T());
}

template <int Dims>
int marray_dims<Dims>::index(marray_dims<Dims> indices) const
{
  int result = 0;
  int multiplier = 1;
  for(int i = Dims - 1; i >= 0; i--)
  {
    result += indices.dims[i] * multiplier;
    multiplier *= dims[i];
  }
  return result;
}

template <class T, int Dims>
template <class... I> T& 
marray<T, Dims>::operator()(I&&... indices)
{
  static_assert(sizeof...(indices) == Dims);
  return (*this)(marray_dims<Dims> { std::forward<I>(indices)... });
}

template <class T, int Dims>
template <class... I> T const& 
marray<T, Dims>::operator()(I&&... indices) const
{
  static_assert(sizeof...(indices) == Dims);
  return (*this)(marray_dims<Dims> { std::forward<I>(indices)... });
}

// jagged array

template <class T, int Dims>
class jarray;
template <class T, int Dims>
struct jarray_traits;

template <class T>
struct jarray_traits<T, 1> final {
  typedef T elem_type;
  typedef int dims_type;
  static void resize(std::vector<elem_type>& v, dims_type const& dims)
  { v.resize(dims); }
};

template <class T, int Dims>
struct jarray_traits final {
  typedef jarray<T, Dims - 1> elem_type;
  typedef jarray<int, Dims - 1> dims_type;
  static void resize(std::vector<elem_type>& v, dims_type const& dims);
};

template <class T, int Dims>
class jarray final {
  typedef typename jarray_traits<T, Dims>::dims_type dims_type;
  typedef typename jarray_traits<T, Dims>::elem_type elem_type;
  std::vector<elem_type> _data;

public:
  INF_DECLARE_MOVE_ONLY(jarray);
  explicit jarray(std::size_t size, elem_type const& val) :
  _data(size, val) {}
  void resize(dims_type const& dims) 
  { jarray_traits<T, Dims>::resize(_data, dims); }

  elem_type& operator[](int i) { return _data[i]; }
  elem_type const& operator[](int i) const { return _data[i]; }

  decltype(_data.end()) end() { return _data.end(); }
  decltype(_data.begin()) begin() { return _data.begin(); }
  decltype(_data.end()) end() const { return _data.end(); }
  decltype(_data.begin()) begin()const { return _data.begin(); }

  void clear() { _data.clear(); }
  std::size_t size() const { return _data.size(); }
  void push_back(elem_type const& val) { _data.push_back(val); }
  template <class... U> decltype(auto) emplace_back(U&&... args) 
  { return _data.emplace_back(std::forward<U>(args)...); }
};

template <class T, int Dims>
void jarray_traits<T, Dims>::resize(std::vector<elem_type>& v, dims_type const& dims)
{
  v.resize(dims.size());
  for (int i = 0; i < dims.size(); i++)
    v[i].resize(dims[i]);
}

}