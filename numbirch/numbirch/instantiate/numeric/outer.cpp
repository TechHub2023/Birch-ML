/**
 * @file
 */
#ifdef BACKEND_CUDA
#include "numbirch/cuda/numeric.inl"
#endif
#ifdef BACKEND_EIGEN
#include "numbirch/eigen/numeric.inl"
#endif

#define OUTER(f) \
    OUTER_SIG(f, real)
#define OUTER_SIG(f, T) \
    template Array<T,2> f(const Array<T,1>&, const Array<T,1>&); \
    template Array<T,2> f(const Array<T,2>&, const Array<T,2>&);

namespace numbirch {
OUTER(outer)
}
