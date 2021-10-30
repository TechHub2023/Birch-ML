/**
 * @file
 * 
 * Explicit template instantiations for special functions used from Eigen.
 * This ensures that Eigen dependency does not propagate to downstream code.
 */
#include "numbirch/function.hpp"

/* for recent versions of CUDA, disables warnings about diag_suppress being
 * deprecated in favor of nv_diag_suppress */
#pragma nv_diag_suppress 20236

#if defined(HAVE_UNSUPPORTED_EIGEN_SPECIALFUNCTIONS)
#include <unsupported/Eigen/SpecialFunctions>
#elif defined(HAVE_EIGEN3_UNSUPPORTED_EIGEN_SPECIALFUNCTIONS)
#include <eigen3/unsupported/Eigen/SpecialFunctions>
#endif

namespace numbirch {

template<class T, std::enable_if_t<std::is_floating_point<T>::value,int>>
HOST_DEVICE T digamma(const T x) {
  return Eigen::numext::digamma(x);
}

template<class T, std::enable_if_t<std::is_floating_point<T>::value,int>>
HOST_DEVICE T gamma_p(const T a, const T x) {
  return Eigen::numext::igamma(a, x);
}

template<class T, std::enable_if_t<std::is_floating_point<T>::value,int>>
HOST_DEVICE T gamma_q(const T a, const T x) {
  return Eigen::numext::igammac(a, x);
}

template<class T, class U, std::enable_if_t<
    std::is_floating_point<T>::value && std::is_arithmetic<U>::value,int>>
HOST_DEVICE T ibeta(const U a, const U b, const T x) {
  /* as of Eigen 3.4.0, the edge cases of a = 0 and b = 0 are not handled
   * internally, see https://gitlab.com/libeigen/eigen/-/issues/2359 */
  if (a == U(0) && b != U(0)) {
    return T(1);
  } else if (a != U(0) && b == U(0)) {
    return T(0);
  } else {
    return Eigen::numext::betainc(T(a), T(b), x);
  }
}

}

template HOST_DEVICE double numbirch::digamma<double,0>(double);
template HOST_DEVICE float numbirch::digamma<float,0>(float);

template HOST_DEVICE double numbirch::gamma_p(double, double);
template HOST_DEVICE float numbirch::gamma_p(float, float);

template HOST_DEVICE double numbirch::gamma_q(double, double);
template HOST_DEVICE float numbirch::gamma_q(float, float);

template HOST_DEVICE double numbirch::ibeta(double, double, double);
template HOST_DEVICE double numbirch::ibeta(int, int, double);
template HOST_DEVICE float numbirch::ibeta(float, float, float);
template HOST_DEVICE float numbirch::ibeta(int, int, float);

