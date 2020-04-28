/*
 * Test normal-inverse-gamma conjugacy.
 */
program test_normal_inverse_gamma(N:Integer <- 10000) {
  m:TestNormalInverseGamma;
  test_conjugacy(m, N, 2);
}

class TestNormalInverseGamma < Model {  
  σ2:Random<Real>;
  x:Random<Real>;

  μ:Real;
  a2:Real;
  α:Real;
  β:Real;
  
  function initialize() {
    μ <- simulate_uniform(-10.0, 10.0);
    a2 <- simulate_uniform(0.1, 2.0);
    α <- simulate_uniform(2.0, 10.0);
    β <- simulate_uniform(0.1, 10.0);
  }
  
  fiber simulate() -> Event {
    σ2 ~ InverseGamma(α, β);
    x ~ Gaussian(μ, a2, σ2);
  }
  
  function forward() -> Real[_] {
    y:Real[2];
    assert !σ2.hasValue();
    y[1] <- σ2.value();
    assert !x.hasValue();
    y[2] <- x.value();
    return y;
  }

  function backward() -> Real[_] {
    y:Real[2];
    assert !x.hasValue();
    y[2] <- x.value();
    assert !σ2.hasValue();
    y[1] <- σ2.value();
    return y;
  }
  
  function marginal() -> Distribution<Real> {
    return x.distribution()!.graft();
  }
}
