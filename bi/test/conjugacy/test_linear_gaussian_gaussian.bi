/*
 * Test linear Gaussian-Gaussian conjugacy.
 */
program test_linear_gaussian_gaussian(N:Integer <- 10000) { 
  m:TestLinearGaussianGaussian;
  test_conjugacy(m, N, 2);
}

class TestLinearGaussianGaussian < Model {
  μ_1:Random<Real>;
  x:Random<Real>;
  a:Real;
  c:Real;
  μ_0:Real;
  σ2_0:Real;
  σ2_1:Real;

  function initialize() {
    a <- simulate_uniform(-2.0, 2.0);
    c <- simulate_uniform(-10.0, 10.0);
    μ_0 <- simulate_uniform(-10.0, 10.0);
    σ2_0 <- simulate_uniform(0.0, 2.0);
    σ2_1 <- simulate_uniform(0.0, 2.0);
  }
  
  fiber simulate() -> Event {
    μ_1 ~ Gaussian(μ_0, σ2_0);
    x ~ Gaussian(a*μ_1 + c, σ2_1);
  }
  
  function forward() -> Real[_] {
    y:Real[2];
    y[1] <- μ_1.value();
    assert !x.hasValue();
    y[2] <- x.value();
    return y;
  }

  function backward() -> Real[_] {
    y:Real[2];
    y[2] <- x.value();
    assert !μ_1.hasValue();
    y[1] <- μ_1.value();
    return y;
  }
  
  function marginal() -> Distribution<Real> {
    return x.distribution()!.graft();
  }
}
