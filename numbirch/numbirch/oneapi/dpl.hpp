/**
 * @file
 * 
 * oneDPL integration.
 */
#pragma once

#include "numbirch/oneapi/sycl.hpp"

#include <oneapi/dpl/execution>
#include <oneapi/dpl/algorithm>
#include <oneapi/dpl/numeric>
#include <oneapi/dpl/ranges>
#include <oneapi/dpl/async>

namespace dpl = oneapi::dpl;

namespace numbirch {

template<class T>
static auto make_dpl_vector(T* x, const int n, const int incx) {
  return dpl::experimental::ranges::transform_view(
      dpl::experimental::ranges::iota_view(0, n), [=](int i) -> T& {
        return x[i*incx];
      });
}

template<class T>
static auto make_dpl_matrix(T* A, const int m, const int n, const int ldA) {
  return dpl::experimental::ranges::transform_view(
      dpl::experimental::ranges::iota_view(0, m*n), [=](int i) -> T& {
        int c = i/m;
        int r = i - c*m;
        return get(A, r, c, ldA);
      });
}

template<class T>
static auto make_dpl_matrix_transpose(T* A, const int m, const int n,
    const int ldA) {
  return dpl::experimental::ranges::transform_view(
      dpl::experimental::ranges::iota_view(0, m*n), [=](int i) -> T& {
        int c = i/m;
        int r = i - c*m;
        return get(A, c, r, ldA);
      });
}

template<class T>
static auto make_dpl_matrix_lower(const T* A, const int m, const int n,
    const int ldA) {
  return dpl::experimental::ranges::transform_view(
      dpl::experimental::ranges::iota_view(0, m*n), [=](int i) -> T {
        int c = i/m;
        int r = i - c*m;
        return (c <= r) ? get(A, r, c, ldA) : 0.0;
      });
}

template<class T>
static auto make_dpl_matrix_symmetric(const T* A, const int m, const int n,
    const int ldA) {
  return dpl::experimental::ranges::transform_view(
      dpl::experimental::ranges::iota_view(0, m*n), [=](int i) -> T {
        int c = i/m;
        int r = i - c*m;
        return (c <= r) ? get(A, r, c, ldA) : get(A, c, r, ldA);
      });
}

}
