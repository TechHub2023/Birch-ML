class TestGaussianGaussian < Model {
  μ_1:Random<Real>;
  x:Random<Real>;
  μ_0:Real;
  σ2_0:Real;
  σ2_1:Real;

  function initialize() {
    μ_0 <- simulate_uniform(-10.0, 10.0);
    σ2_0 <- simulate_uniform(0.0, 10.0);
    σ2_1 <- simulate_uniform(0.0, 10.0);
  }

  fiber simulate() -> Event {
    μ_1 ~ Gaussian(μ_0, σ2_0);
    x ~ Gaussian(μ_1, σ2_1);
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
}