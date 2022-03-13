/**
 * @file
 */
#ifdef BACKEND_CUDA
#include "numbirch/cuda/transform.hpp"
#include "numbirch/cuda/random.hpp"
#endif
#ifdef BACKEND_EIGEN
#include "numbirch/eigen/transform.hpp"
#include "numbirch/eigen/random.hpp"
#endif
#include "numbirch/common/transform.hpp"
#include "numbirch/common/random.hpp"

#define RANDOM_UNARY(f, R) \
    RANDOM_UNARY_FIRST(f, R, real) \
    RANDOM_UNARY_FIRST(f, R, int) \
    RANDOM_UNARY_FIRST(f, R, bool)
#define RANDOM_UNARY_FIRST(f, R, T) \
    RANDOM_UNARY_SIG(f, R, ARRAY(T, 2)) \
    RANDOM_UNARY_SIG(f, R, ARRAY(T, 1)) \
    RANDOM_UNARY_SIG(f, R, ARRAY(T, 0)) \
    RANDOM_UNARY_SIG(f, R, T)
#define RANDOM_UNARY_SIG(f, R, T) \
    template R<T> f<T,int>(const T&);

#define RANDOM_UNARY_REAL(f) RANDOM_UNARY(f, real_t)
#define RANDOM_UNARY_INT(f) RANDOM_UNARY(f, int_t)
#define RANDOM_UNARY_BOOL(f) RANDOM_UNARY(f, bool_t)

namespace numbirch {
RANDOM_UNARY_BOOL(simulate_bernoulli)
RANDOM_UNARY_REAL(simulate_chi_squared)
RANDOM_UNARY_REAL(simulate_exponential)
RANDOM_UNARY_INT(simulate_poisson)
RANDOM_UNARY_REAL(simulate_student_t)
}