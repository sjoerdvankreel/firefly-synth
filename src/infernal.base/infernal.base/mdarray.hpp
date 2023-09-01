#pragma once

#include <vector>

namespace infernal::base {

template <class T>
class array2d {
  std::vector<T*> _dim0 = {};
  std::vector<T> _dim1 = {};
public:

  T* const* operator[](int i) 
  { return _dim0[i]; }
  T const* const* operator[](int i) const
  { return _dim0[i]; }

  void clear()
  {
    _dim0.clear();
    _dim1.clear();
  }

  void init(int dim0, int dim1) 
  {
    clear();
    _dim0.resize(dim0);
    _dim1.resize(dim0 * dim1);
    for(int i0 = 0; i0 < dim0; i0++)
      _dim0[i0] = _dim1.data() + i0 * dim1;
  }
};

template <class T>
class array3d {
  std::vector<T**> _dim0 = {};
  std::vector<T*> _dim1 = {};
  std::vector<T> _dim2 = {};
public:

  T* const* const* operator[](int i) 
  { return _dim0[i]; }
  T const* const* const* operator[](int i) const 
  { return _dim0[i]; }

  void clear() 
  {
    _dim0.clear();
    _dim1.clear();
    _dim2.clear();
  }

  void init(int dim0, int dim1, int dim2) 
  {
    clear();
    _dim0.resize(dim0);
    _dim1.resize(dim0 * dim1);
    _dim2.resize(dim0 * dim1 * dim2);
    for (int i1 = 0; i1 < dim0 * dim1; i1++)
      _dim1[i1] = _dim2.data() + i1 * dim2;
    for(int i0 = 0; i0 < dim0; i0++)
      _dim0[i0] = _dim1.data() + i0 * dim1;
  }
};

template <class T>
class array4d {
  std::vector<T***> _dim0 = {};
  std::vector<T**> _dim1 = {};
  std::vector<T*> _dim2 = {};
  std::vector<T> _dim3 = {};
public:

  T* const* const* const* operator[](int i) 
  { return _dim0[i]; }
  T const* const* const* const* operator[](int i) const 
  { return _dim0[i]; }

  void clear() 
  {
    _dim0.clear();
    _dim1.clear();
    _dim2.clear();
    _dim3.clear();
  }

  void init(int dim0, int dim1, int dim2, int dim3) 
  {
    clear();
    _dim0.resize(dim0);
    _dim1.resize(dim0 * dim1);
    _dim2.resize(dim0 * dim1 * dim2);
    _dim3.resize(dim0 * dim1 * dim2 * dim3);
    for(int i2 = 0; i2 < dim0 * dim1 * dim2; i2++)
      _dim2[i2] = _dim3.data() + i2 * dim3;
    for (int i1 = 0; i1 < dim0 * dim1; i1++)
      _dim1[i1] = _dim2.data() + i1 * dim2;
    for(int i0 = 0; i0 < dim0; i0++)
      _dim0[i0] = _dim1.data() + i0 * dim1;
  }
};

}