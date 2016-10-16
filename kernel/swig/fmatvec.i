// check target language
#ifndef SWIGPYTHON
  #error "Only Pyhton as target language is supported."
#endif

// add code to the generated code
%{

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include <typeinfo>
#include <fmatvec/atom.h>
#include <fmatvec/range.h>

template<typename AT> void checkNumPyType(int type);
template<> void checkNumPyType<int>(int type) {
  if(type!=NPY_SHORT    && type!=NPY_USHORT    &&
     type!=NPY_INT      && type!=NPY_UINT      &&
     type!=NPY_LONG     && type!=NPY_ULONG     &&
     type!=NPY_LONGLONG && type!=NPY_ULONGLONG)
    throw std::runtime_error("Value is not of type integer.");
}
template<> void checkNumPyType<double>(int type) {
  if(type!=NPY_SHORT    && type!=NPY_USHORT    &&
     type!=NPY_INT      && type!=NPY_UINT      &&
     type!=NPY_LONG     && type!=NPY_ULONG     &&
     type!=NPY_LONGLONG && type!=NPY_ULONGLONG &&
     type!=NPY_FLOAT    && type!=NPY_DOUBLE    && type!=NPY_LONGDOUBLE)
    throw std::runtime_error("Value is not of type floating point.");
}

template<typename AT> AT arrayGet(PyArrayObject *a, int type, int r, int c=-1);
template<> int arrayGet<int>(PyArrayObject *a, int type, int r, int c) {
  switch(type) {
    case NPY_SHORT:      return *static_cast<npy_short*>     (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_USHORT:     return *static_cast<npy_ushort*>    (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_INT:        return *static_cast<npy_int*>       (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_UINT:       return *static_cast<npy_uint*>      (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_LONG:       return *static_cast<npy_long*>      (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_ULONG:      return *static_cast<npy_ulong*>     (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_LONGLONG:   return *static_cast<npy_longlong*>  (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_ULONGLONG:  return *static_cast<npy_ulonglong*> (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
  }
  throw std::runtime_error("Value is not of type floating point (wrong element type).");
}
template<> double arrayGet<double>(PyArrayObject *a, int type, int r, int c) {
  switch(type) {
    case NPY_SHORT:      return *static_cast<npy_short*>     (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_USHORT:     return *static_cast<npy_ushort*>    (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_INT:        return *static_cast<npy_int*>       (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_UINT:       return *static_cast<npy_uint*>      (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_LONG:       return *static_cast<npy_long*>      (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_ULONG:      return *static_cast<npy_ulong*>     (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_LONGLONG:   return *static_cast<npy_longlong*>  (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_ULONGLONG:  return *static_cast<npy_ulonglong*> (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_FLOAT:      return *static_cast<npy_float*>     (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_DOUBLE:     return *static_cast<npy_double*>    (c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
    case NPY_LONGDOUBLE: return *static_cast<npy_longdouble*>(c==-1 ? PyArray_GETPTR1(a, r) : PyArray_GETPTR2(a, r, c));
  }
  throw std::runtime_error("Value is not of type floating point (wrong element type).");
}

template<class T> inline T& derefIfPointer(T & t) { return  t; }
template<class T> inline T& derefIfPointer(T*& t) { return *t; }

template<class T> inline void assignIfPointer(T & d, T& s) {}
template<class T> inline void assignIfPointer(T*& d, T& s) { d=&s; }

template<class T> inline void assignOrCopy(T & d, T* s) { d=*s; }
template<class T> inline void assignOrCopy(T*& d, T* s) { d= s; }

template<typename AT> constexpr int numPyType();
template<> constexpr int numPyType<int>() { return NPY_LONG; }
template<> constexpr int numPyType<double>() { return NPY_DOUBLE; }

%}

// init numpy
%init %{
  _import_array();
%}

// use natural vars for every member
%naturalvar;

%import "std_string.i"

// use SWIG_exception to throw a target language exception
%include exception.i

// helper define to convert c++ exceptions in typemaps to target language exceptions
%define FMATVEC_CATCHARG
  catch(const std::exception &ex) {
    Py_XDECREF($result);
    SWIG_exception(SWIG_RuntimeError, (std::string("In function $symname, argument $argnum: ")+ex.what()).c_str());
  }
%enddef

// fmatvec typedefs and template instantiations
%include <fmatvec/fmatvec.h>

%template() fmatvec::Vector   <fmatvec::Ref      , int   >;
%template() fmatvec::Vector   <fmatvec::Var      , int   >;
%template() fmatvec::Vector   <fmatvec::Fixed< 1>, int   >;
%template() fmatvec::Vector   <fmatvec::Fixed< 2>, int   >;
%template() fmatvec::Vector   <fmatvec::Fixed< 3>, int   >;
%template() fmatvec::Vector   <fmatvec::Ref      , double>;
%template() fmatvec::Vector   <fmatvec::Var      , double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 1>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 2>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 3>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 4>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 5>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 6>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 7>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 8>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed< 9>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<10>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<11>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<12>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<13>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<14>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<15>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<16>, double>;
%template() fmatvec::Vector   <fmatvec::Fixed<17>, double>;
%template() fmatvec::RowVector<fmatvec::Var      , int   >;
%template() fmatvec::RowVector<fmatvec::Ref      , double>;
%template() fmatvec::RowVector<fmatvec::Var      , double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 1>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 2>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 3>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 4>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 5>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 6>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 7>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 8>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed< 9>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<10>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<11>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<12>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<13>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<14>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<15>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<16>, double>;
%template() fmatvec::RowVector<fmatvec::Fixed<17>, double>;

%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Var      , int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 1>, int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 2>, int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 3>, int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 4>, int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Var      , int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Var      , int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Var      , int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Var      , int   >;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Ref      , fmatvec::Ref      , double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Var      , double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Var      , fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Var      , double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Var      , double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Var      , double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Var      , double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 1>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 2>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 3>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 4>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 5>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 6>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 7>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 8>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed< 9>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<10>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<11>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<12>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<13>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<14>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<15>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<16>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 5>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 6>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 7>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 8>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed< 9>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<10>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<11>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<12>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<13>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<14>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<15>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<16>, double>;
%template() fmatvec::Matrix<fmatvec::General    , fmatvec::Fixed<17>, fmatvec::Fixed<17>, double>;
%template() fmatvec::Matrix<fmatvec::Rotation   , fmatvec::Fixed< 3>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::Symmetric  , fmatvec::Ref      , fmatvec::Ref      , double>;
%template() fmatvec::Matrix<fmatvec::Symmetric  , fmatvec::Var      , fmatvec::Var      , double>;
%template() fmatvec::Matrix<fmatvec::Symmetric  , fmatvec::Fixed< 1>, fmatvec::Fixed< 1>, double>;
%template() fmatvec::Matrix<fmatvec::Symmetric  , fmatvec::Fixed< 2>, fmatvec::Fixed< 2>, double>;
%template() fmatvec::Matrix<fmatvec::Symmetric  , fmatvec::Fixed< 3>, fmatvec::Fixed< 3>, double>;
%template() fmatvec::Matrix<fmatvec::Symmetric  , fmatvec::Fixed< 4>, fmatvec::Fixed< 4>, double>;
%template() fmatvec::Matrix<fmatvec::Symmetric  , fmatvec::Fixed< 5>, fmatvec::Fixed< 5>, double>;

//MFMF not working %template() fmatvec::Matrix<fmatvec::Sparse     , fmatvec::Ref      , fmatvec::Ref      , double>;
//MFMF not working %template() fmatvec::Matrix<fmatvec::Diagonal   , fmatvec::Ref      , fmatvec::Ref      , double>;
//MFMF not working %template() fmatvec::Matrix<fmatvec::GeneralBand, fmatvec::Ref      , fmatvec::Ref      , double>;

%template() fmatvec::SquareMatrix<fmatvec::Ref      , double>;
%template() fmatvec::SquareMatrix<fmatvec::Var      , double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 1>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 2>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 3>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 4>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 5>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 6>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 7>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 8>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed< 9>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<10>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<11>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<12>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<13>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<14>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<15>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<16>, double>;
%template() fmatvec::SquareMatrix<fmatvec::Fixed<17>, double>;

// wrap the following
%include <fmatvec/range.h>
%template(Index) fmatvec::Range<fmatvec::Var,fmatvec::Var>;

%typemap(out) fmatvec::Vector,
              const fmatvec::Vector,
              fmatvec::RowVector,
              const fmatvec::RowVector {
  try {
    npy_intp dims[1];
    dims[0]=$1.size();
    $result=PyArray_SimpleNew(1, dims, numPyType<$1_basetype::AtomicType>());
    if(!$result)
      throw std::runtime_error("Cannot create ndarray");
    std::copy(&$1(0), &$1(0)+$1.size(),
              static_cast<$1_basetype::AtomicType*>(PyArray_GETPTR1(reinterpret_cast<PyArrayObject*>($result), 0)));
  }
  FMATVEC_CATCHARG
}

%typemap(out) const fmatvec::Vector&,
              const fmatvec::RowVector& {
  try {
    std::string symname("$symname");
    if(symname.substr(symname.size()-4)=="_get") {//MISSING is not working if a function ends with "_get"
      npy_intp dims[1];
      dims[0]=$1->size();
      $result=PyArray_SimpleNewFromData(1, dims, numPyType<$1_basetype::AtomicType>(), &(*$1)(0));
      if(!$result)
        throw std::runtime_error("Cannot create ndarray");
    }
    else {
      npy_intp dims[1];
      dims[0]=$1->size();
      $result=PyArray_SimpleNew(1, dims, numPyType<$1_basetype::AtomicType>());
      if(!$result)
        throw std::runtime_error("Cannot create ndarray");
      std::copy(&(*$1)(0), &(*$1)(0)+$1->size(),
                static_cast<$1_basetype::AtomicType*>(PyArray_GETPTR1(reinterpret_cast<PyArrayObject*>($result), 0)));
    }
  }
  FMATVEC_CATCHARG
}

%typemap(out) fmatvec::Vector&,
              fmatvec::RowVector& {
  try {
    npy_intp dims[1];
    dims[0]=$1->size();
    $result=PyArray_SimpleNewFromData(1, dims, numPyType<$1_basetype::AtomicType>(), &(*$1)(0));
    if(!$result)
      throw std::runtime_error("Cannot create ndarray");
  }
  FMATVEC_CATCHARG
}

%typemap(arginit, noblock=1) fmatvec::Vector,
                             const fmatvec::Vector,
                             fmatvec::Vector&,
                             const fmatvec::Vector&,
                             fmatvec::RowVector,
                             const fmatvec::RowVector,
                             fmatvec::RowVector&,
                             const fmatvec::RowVector& {
  $1_basetype localVar$argnum;
}

%typemap(in) fmatvec::Vector,
             const fmatvec::Vector,
             fmatvec::Vector&,
             const fmatvec::Vector&,
             fmatvec::RowVector,
             const fmatvec::RowVector,
             fmatvec::RowVector&,
             const fmatvec::RowVector& {
  try {
    void *inputp;
    int res=SWIG_ConvertPtr($input, &inputp, $1_descriptor, 0);
    if(SWIG_IsOK(res))
      assignOrCopy($1, reinterpret_cast<$1_basetype*>(inputp));
    else if(PyArray_Check($input)) {
      PyArrayObject *input=reinterpret_cast<PyArrayObject*>($input);
      if(PyArray_NDIM(input)!=1)
        throw std::runtime_error("Must have 1 dimension.");
      int type=PyArray_TYPE(input);
      checkNumPyType<$1_basetype::AtomicType>(type);
      npy_intp *dims=PyArray_SHAPE(input);
      assignIfPointer($1, localVar$argnum);
      derefIfPointer($1).resize(dims[0]);
      for(int i=0; i<dims[0]; ++i)
        derefIfPointer($1)(i)=arrayGet<$1_basetype::AtomicType>(input, type, i);
    }
    else
      throw std::runtime_error("Wrong type.");
  }
  FMATVEC_CATCHARG
}

%typemap(argout) fmatvec::Vector,
                 const fmatvec::Vector,
                 const fmatvec::Vector&,
                 fmatvec::RowVector,
                 const fmatvec::RowVector,
                 const fmatvec::RowVector& {
}

%typemap(argout) fmatvec::Vector&,
                 fmatvec::RowVector& {
  try {
    PyArrayObject *input=reinterpret_cast<PyArrayObject*>($input);
    int type=PyArray_TYPE(input);
    if(type!=numPyType<$1_basetype::AtomicType>())
      throw std::runtime_error(std::string("Must have atomic type ")+typeid($1_basetype::AtomicType).name());
    std::copy(&localVar$argnum(0), &localVar$argnum(0)+localVar$argnum.size(),
              static_cast<$1_basetype::AtomicType*>(PyArray_GETPTR1(input, 0)));
  }
  FMATVEC_CATCHARG
}

%typemap(out) fmatvec::Matrix,
              const fmatvec::Matrix,
              fmatvec::SquareMatrix,
              const fmatvec::SquareMatrix {
  try {
    npy_intp dims[2];
    dims[0]=derefIfPointer($1).rows();
    dims[1]=derefIfPointer($1).cols();
    $result=PyArray_SimpleNew(2, dims, numPyType<$1_basetype::AtomicType>());
    if(!$result)
      throw std::runtime_error("Cannot create ndarray");
    PyArrayObject *input=reinterpret_cast<PyArrayObject*>($result);
    for(int r=0; r<dims[0]; r++)
      for(int c=0; c<dims[1]; c++)
        *static_cast<$1_basetype::AtomicType*>(PyArray_GETPTR2(input, r, c))=
          derefIfPointer($1)(r, c);
  }
  FMATVEC_CATCHARG
}

%typemap(out) const fmatvec::Matrix&,
              const fmatvec::SquareMatrix& {
  try {
    std::string symname("$symname");
    if(symname.substr(symname.size()-4)=="_get") {//MISSING is not working if a function ends with "_get"
      npy_intp dims[2];
      dims[0]=$1->rows();
      dims[1]=$1->cols();
      $result=PyArray_SimpleNewFromData(2, dims, numPyType<$1_basetype::AtomicType>(), &(*$1)(0, 0));
      if(!$result)
        throw std::runtime_error("Cannot create ndarray");
      npy_intp *strides=PyArray_STRIDES(reinterpret_cast<PyArrayObject*>($result));
      strides[$1->transposed() ? 1 : 0]=sizeof($1_basetype::AtomicType)*1;
      strides[$1->transposed() ? 0 : 1]=sizeof($1_basetype::AtomicType)*$1->ldim();
    }
    else {
      npy_intp dims[2];
      dims[0]=$1->rows();
      dims[1]=$1->cols();
      $result=PyArray_SimpleNew(2, dims, numPyType<$1_basetype::AtomicType>());
      if(!$result)
        throw std::runtime_error("Cannot create ndarray");
      PyArrayObject *input=reinterpret_cast<PyArrayObject*>($result);
      for(int r=0; r<dims[0]; r++)
        for(int c=0; c<dims[1]; c++)
          *static_cast<$1_basetype::AtomicType*>(PyArray_GETPTR2(input, r, c))=
            (*$1)(r, c);
    }
  }
  FMATVEC_CATCHARG
}

%typemap(out) fmatvec::Matrix&,
              fmatvec::SquareMatrix& {
  try {
    npy_intp dims[2];
    dims[0]=$1->rows();
    dims[1]=$1->cols();
    $result=PyArray_SimpleNewFromData(2, dims, numPyType<$1_basetype::AtomicType>(), &(*$1)(0, 0));
    if(!$result)
      throw std::runtime_error("Cannot create ndarray");
    npy_intp *strides=PyArray_STRIDES(reinterpret_cast<PyArrayObject*>($result));
    strides[$1->transposed() ? 1 : 0]=sizeof($1_basetype::AtomicType)*1;
    strides[$1->transposed() ? 0 : 1]=sizeof($1_basetype::AtomicType)*$1->ldim();
  }
  FMATVEC_CATCHARG
}

%typemap(arginit, noblock=1) fmatvec::Matrix,
                             const fmatvec::Matrix,
                             fmatvec::Matrix&,
                             const fmatvec::Matrix&,
                             fmatvec::SquareMatrix,
                             const fmatvec::SquareMatrix,
                             fmatvec::SquareMatrix&,
                             const fmatvec::SquareMatrix& {
  $1_basetype localVar$argnum;
}

%typemap(in) fmatvec::Matrix,
             const fmatvec::Matrix,
             fmatvec::Matrix&,
             const fmatvec::Matrix&,
             fmatvec::SquareMatrix,
             const fmatvec::SquareMatrix,
             fmatvec::SquareMatrix&,
             const fmatvec::SquareMatrix& {
  try {
    void *inputp;
    int res=SWIG_ConvertPtr($input, &inputp, $1_descriptor, 0);
    if(SWIG_IsOK(res))
      assignOrCopy($1, reinterpret_cast<$1_basetype*>(inputp));
    else if(PyArray_Check($input)) {
      PyArrayObject *input=reinterpret_cast<PyArrayObject*>($input);
      if(PyArray_NDIM(input)!=2)
        throw std::runtime_error("Must have 2 dimension.");
      int type=PyArray_TYPE(input);
      checkNumPyType<$1_basetype::AtomicType>(type);
      assignIfPointer($1, localVar$argnum);
      npy_intp *dims=PyArray_SHAPE(input);
      derefIfPointer($1).resize(dims[0], dims[1]);
      for(int r=0; r<dims[0]; ++r)
        for(int c=0; c<dims[1]; ++c)
          derefIfPointer($1)(r, c)=arrayGet<$1_basetype::AtomicType>(input, type, r, c);
    }
    else
      throw std::runtime_error("Wrong type.");
  }
  FMATVEC_CATCHARG
}

%typemap(argout) fmatvec::Matrix,
                 const fmatvec::Matrix,
                 const fmatvec::Matrix&,
                 fmatvec::SquareMatrix,
                 const fmatvec::SquareMatrix,
                 const fmatvec::SquareMatrix& {
}

%typemap(argout) fmatvec::Matrix&,
                 fmatvec::SquareMatrix& {
  try {
    PyArrayObject *input=reinterpret_cast<PyArrayObject*>($input);
    int type=PyArray_TYPE(input);
    if(type!=numPyType<$1_basetype::AtomicType>())
      throw std::runtime_error(std::string("Must have atomic type ")+typeid($1_basetype::AtomicType).name());
    for(int r=0; r<localVar$argnum.rows(); ++r)
      for(int c=0; c<localVar$argnum.cols(); ++c)
        *static_cast<$1_basetype::AtomicType*>(PyArray_GETPTR2(input, r, c))=
          localVar$argnum(r, c);
  }
  FMATVEC_CATCHARG
}

// wrap fmatvec::Atom
%feature("director") fmatvec::Atom;
namespace fmatvec {
  %extend Atom {
    void msg(MsgType type, const std::string &msg) {
      $self->msg(type)<<msg<<std::endl;
    }
    static void msgStatic(MsgType type, const std::string &msg) {
      fmatvec::Atom::msgStatic(type)<<msg<<std::endl;
    }
  };
}
%ignore fmatvec::Atom::setCurrentMessageStream;
%ignore fmatvec::Atom::setMessageStreamActive;
%ignore fmatvec::Atom::getMessageStream;
%ignore fmatvec::Atom::adoptMessageStreams;
%ignore fmatvec::Atom::msg;
%ignore fmatvec::Atom::msgStatic;
%include <fmatvec/atom.h>
