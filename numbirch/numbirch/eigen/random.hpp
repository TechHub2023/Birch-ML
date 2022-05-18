/**
 * @file
 */
#pragma once

#include "numbirch/random.hpp"
#include "numbirch/common/stl.hpp"
#include "numbirch/common/get.hpp"

namespace numbirch {

struct simulate_bernoulli_functor {
  bool operator()(const real rho) const {
    return std::bernoulli_distribution(rho)(stl<bool>::rng());
  }
};

struct simulate_beta_functor {
  real operator()(const real alpha, const real beta) const {
    real u, v;
    auto& rng = stl<real>::rng();
    u = std::gamma_distribution<real>(alpha)(rng);
    v = std::gamma_distribution<real>(beta)(rng);
    return u/(u + v);
  }
};

struct simulate_binomial_functor {
  int operator()(const int n, const real rho) const {
    return std::binomial_distribution<int>(n, rho)(stl<int>::rng());
  }
};

struct simulate_chi_squared_functor {
  real operator()(const real nu) const {
    return std::chi_squared_distribution<real>(nu)(stl<real>::rng());
  }
};

struct simulate_exponential_functor {
  real operator()(const real lambda) const {
    return std::exponential_distribution<real>(lambda)(stl<real>::rng());
  }
};

struct simulate_gamma_functor {
  real operator()(const real k, const real theta) const {
    return std::gamma_distribution<real>(k, theta)(stl<real>::rng());
  }
};

struct simulate_gaussian_functor {
  real operator()(const real mu, const real sigma2) const {
    real sigma = std::sqrt(sigma2);
    return std::normal_distribution<real>(mu, sigma)(stl<real>::rng());
  }
};

struct simulate_negative_binomial_functor {
  int operator()(const int k, const real rho) const {
    return std::negative_binomial_distribution<int>(k, rho)(stl<int>::rng());
  }
};

struct simulate_poisson_functor {
  int operator()(const real lambda) const {
    return std::poisson_distribution<int>(lambda)(stl<int>::rng());
  }
};

struct simulate_uniform_functor {
  real operator()(const real l, const real u) const {
    return std::uniform_real_distribution<real>(l, u)(stl<real>::rng());
  }
};

struct simulate_uniform_int_functor {
  int operator()(const int l, const int u) const {
    return std::uniform_int_distribution<int>(l, u)(stl<int>::rng());
  }
};

struct simulate_weibull_functor {
  real operator()(const real k, const real lambda) const {
    return std::weibull_distribution<real>(k, lambda)(stl<real>::rng());
  }
};

struct standard_gaussian_functor {
  NUMBIRCH_HOST_DEVICE real operator()(const int i, const int j) const {
    return std::normal_distribution<real>()(stl<real>::rng());
  }
};

template<class T>
struct standard_wishart_functor {
  T k;
  int n;
  standard_wishart_functor(const T& k, const int n) :
      k(k),
      n(n) {
    //
  }
  NUMBIRCH_HOST_DEVICE real operator()(const int i, const int j) {
    auto& rng = stl<real>::rng();
    if (i == j) {
      /* on diagonal */
      real nu = get(k) + (n - 1 - i); // i is 0-based here
      real x = std::chi_squared_distribution<real>(nu)(rng);
      return std::sqrt(x);
    } else if (i > j) {
      /* in lower triangle */
      return std::normal_distribution<real>()(rng);
    } else {
      /* in upper triangle */
      return real(0.0);
    }
  }
};
}