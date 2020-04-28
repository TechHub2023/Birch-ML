/*
 * Test a particular conjugacy.
 *
 * - m: Model.
 * - N: Number of samples.
 * - D: Number of dimensions.
 */
function test_conjugacy<Conjugate>(m:Conjugate, N:Integer, D:Integer) {  
  auto forward <- PlayHandler(false);
  auto backward <- PlayHandler(true);

  m.initialize();
  
  /* simulate forward */
  X1:Real[N,D];
  for n in 1..N {
    auto m' <- clone(m);
    forward.handle(m'.simulate());
    X1[n,1..D] <- m'.forward();
  }

  /* simulate backward */
  X2:Real[N,D];
  for n in 1..N {
    auto m' <- clone(m);
    backward.handle(m'.simulate());
    X2[n,1..D] <- m'.backward();
  }
  
  /* test result */
  if !pass(X1, X2) {
    exit(1);
  }
}
