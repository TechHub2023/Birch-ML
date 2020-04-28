/*
 * Test multivariate normal-inverse-gamma-Gaussian conjugacy.
 */
program test_multivariate_normal_inverse_gamma_multivariate_gaussian(N:Integer <- 10000) {
  m:TestMultivariateNormalInverseGammaMultivariateGaussian;
  test_conjugacy(m, N, 11);
}

class TestMultivariateNormalInverseGammaMultivariateGaussian < Model {
  σ2:Random<Real>;
  μ:Random<Real[_]>;
  x:Random<Real[_]>;

  μ_0:Real[5];
  A:Real[5,5];
  α:Real;
  β:Real;
  
  function initialize() {
    α <- simulate_uniform(2.0, 10.0);
    β <- simulate_uniform(0.0, 10.0); 
    for i in 1..5 {
      μ_0[i] <- simulate_uniform(-10.0, 10.0);
      for j in 1..5 {
        A[i,j] <- simulate_uniform(-2.0, 2.0);
      }
    }
    A <- A*transpose(A);
  }
  
  fiber simulate() -> Event {
    σ2 ~ InverseGamma(α, β);
    μ ~ Gaussian(μ_0, A, σ2);
    x ~ Gaussian(μ, σ2);
  }
  
  function forward() -> Real[_] {
    y:Real[11];
    y[1] <- σ2.value();
    assert !μ.hasValue();
    y[2..6] <- μ.value();
    assert !x.hasValue();
    y[7..11] <- x.value();
    return y;
  }

  function backward() -> Real[_] {
    y:Real[11];
    y[7..11] <- x.value();
    assert !μ.hasValue();
    y[2..6] <- μ.value();
    assert !σ2.hasValue();
    y[1] <- σ2.value();
    return y;
  }
  
  function marginal() -> Distribution<Real[_]> {
    return x.distribution()!.graft();
  }
}
