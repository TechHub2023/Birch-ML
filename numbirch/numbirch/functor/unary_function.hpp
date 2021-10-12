/**
 * @file
 */
#pragma once

#include "numbirch/function.hpp"

namespace numbirch {

template<class T>
struct abs_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::abs(x);
  }
};

template<class T>
struct acos_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::acos(x);
  }
};

template<class T>
struct asin_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::asin(x);
  }
};

template<class T>
struct atan_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::atan(x);
  }
};

template<class T>
struct ceil_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::ceil(x);
  }
};

template<class T>
struct cos_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::cos(x);
  }
};

template<class T>
struct cosh_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::cosh(x);
  }
};

template<class T>
struct digamma_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return digamma(x);
  }
};

template<class T>
struct exp_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::exp(x);
  }
};

template<class T>
struct expm1_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::expm1(x);
  }
};

template<class T>
struct floor_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::floor(x);
  }
};

template<class T>
struct lgamma_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::lgamma(x);
  }
};

template<class T>
struct log_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::log(x);
  }
};

template<class T>
struct log1p_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::log1p(x);
  }
};

template<class T>
struct log_abs_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::log(std::abs(x));
  }
};

template<class T>
struct log_square_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return 2.0*std::log(x);
  }
};

template<class T>
struct rectify_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return rectify(x);
  }
};

template<class T>
struct round_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::round(x);
  }
};

template<class T>
struct sin_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::sin(x);
  }
};

template<class T>
struct sinh_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::sinh(x);
  }
};

template<class T>
struct sqrt_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::sqrt(x);
  }
};

template<class T>
struct tan_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::tan(x);
  }
};

template<class T>
struct tanh_functor {
  HOST_DEVICE auto operator()(const T x) const {
    return std::tanh(x);
  }
};

}