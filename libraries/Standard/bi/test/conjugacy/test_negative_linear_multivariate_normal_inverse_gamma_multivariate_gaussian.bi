/*
 * Test multivariate linear normal-inverse-gamma-Gaussian conjugacy.
 */
program test_negative_linear_multivariate_normal_inverse_gamma_multivariate_gaussian(
    N:Integer <- 10000) {
  m:TestNegativeLinearMultivariateNormalInverseGammaMultivariateGaussian;
  test_conjugacy(m, N, 16);
}