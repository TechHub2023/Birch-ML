/**
 * @file
 */
#include "numbirch/numbirch.hpp"

#include <cublas_v2.h>
#include <cusolverDn.h>

#include <thrust/execution_policy.h>
#include <thrust/transform.h>
#include <thrust/transform_reduce.h>
#include <thrust/inner_product.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/async/copy.h>

/**
 * @def CUDA_CHECK
 * 
 * Call a cuda* function and assert success.
 */
#define CUDA_CHECK(call) \
    { \
      cudaError_t err = call; \
      assert(err == cudaSuccess); \
    }

/**
 * @def CUBLAS_CHECK
 * 
 * Call a cublas* function and assert success.
 */
#define CUBLAS_CHECK(call) \
    { \
      cublasStatus_t err = call; \
      assert(err == CUBLAS_STATUS_SUCCESS); \
    }

/**
 * @def CUSOLVER_CHECK
 * 
 * Call a cusolver* function and assert success.
 */
#define CUSOLVER_CHECK(call) \
    { \
      cusolverStatus_t err = call; \
      assert(err == CUSOLVER_STATUS_SUCCESS); \
    }

static thread_local cudaStream_t stream = cudaStreamPerThread;
static thread_local cublasHandle_t cublasHandle;
static thread_local cusolverDnHandle_t cusolverDnHandle;
static thread_local cusolverDnParams_t cusolverDnParams;
static thread_local auto policy = thrust::device;

/*
 * Thrust support for lambda functions has some limitations as of CUDA 11.4.
 * In particular the cnvcc ommand-line option --extended-lambda may be
 * necessary, and even with that the use of lambda functions within functions
 * with deduced return types is not supported. For this reason we use functors
 * instead of lambda functions, declared below.
 */

template<class T>
struct VectorElementFunctor : public thrust::unary_function<int,T&> {
  VectorElementFunctor(T* x, int incx) :
      x(x),
      incx(incx) {
    //
  }
  __host__ __device__
  T& operator()(const int i) {
    return x[i*incx];
  }
  T* x;
  int incx;
};

template<class T>
struct MatrixElementFunctor : public thrust::unary_function<int,T&> {
  MatrixElementFunctor(T* A, int m, int ldA) :
      A(A),
      m(m),
      ldA(ldA) {
    //
  }
  __host__ __device__
  T& operator()(const int i) {
    int c = i/m;
    int r = i - c*m;
    return A[r + c*ldA];
  }
  T* A;
  int m;
  int ldA;
};

template<class T>
struct MatrixTransposeElementFunctor : public thrust::unary_function<int,T&> {
  MatrixTransposeElementFunctor(T* A, int m, int ldA) :
      A(A),
      m(m),
      ldA(ldA) {
    //
  }
  __host__ __device__
  T& operator()(const int i) {
    int c = i/m;
    int r = i - c*m;
    return A[c + r*ldA];
  }
  T* A;
  int m;
  int ldA;
};

template<class T>
struct MatrixLowerElementFunctor : public thrust::unary_function<int,T> {
  MatrixLowerElementFunctor(const T* A, int m, int ldA) :
      A(A),
      m(m),
      ldA(ldA) {
    //
  }
  __host__ __device__
  T operator()(const int i) {
    int c = i/m;
    int r = i - c*m;
    return (c <= r) ? A[r + c*ldA] : 0.0;
  }
  const T* A;
  int m;
  int ldA;
};

template<class T>
struct MatrixSymmetricElementFunctor : public thrust::unary_function<int,T> {
  MatrixSymmetricElementFunctor(const T* A, int m, int ldA) :
      A(A),
      m(m),
      ldA(ldA) {
    //
  }
  __host__ __device__
  T operator()(const int i) {
    int c = i/m;
    int r = i - c*m;
    return A[(c <= r) ? (r + c*ldA) : (c + r*ldA)];
  }
  const T* A;
  int m;
  int ldA;
};

template<class T = double>
struct MatrixIdentityElementFunctor : public thrust::unary_function<int,T> {
  MatrixIdentityElementFunctor(int m) :
      m(m) {
    //
  }
  __host__ __device__
  T operator()(const int i) const {
    int c = i/m;
    int r = i - c*m;
    return (r == c) ? 1.0 : 0.0;
  }
  int m;
};

template<class T = double>
struct ScalarMultiplyFunctor : public thrust::unary_function<T,T> {
  ScalarMultiplyFunctor(T a) :
      a(a) {
    //
  }
  __host__ __device__
  T operator()(const T x) const {
    return x*a;
  }
  T a;
};

template<class T = double>
struct ScalarDivideFunctor : public thrust::unary_function<T,T> {
  ScalarDivideFunctor(T a) :
      a(a) {
    //
  }
  __host__ __device__
  T operator()(const T x) const {
    return x/a;
  }
  T a;
};

template<class T = double>
struct LogAbsFunctor : public thrust::unary_function<T,T> {
  __host__ __device__
  T operator()(const T x) const {
    return std::log(std::abs(x));
  }
};

template<class T = double>
struct LogFunctor : public thrust::unary_function<T,T> {
  __host__ __device__
  T operator()(const T x) const {
    return std::log(x);
  }
};

/*
 * Simplified handling of pairs of iterators that define a range of a Thrust
 * vector or matrix.
 */

template<class Iterator>
struct ThrustRange {
  ThrustRange(Iterator first, Iterator second) :
      first(first),
      second(second) {
    //
  }
  auto begin() {
    return first;
  }
  auto end() {
    return second;
  }
  Iterator first;
  Iterator second;
};

template<class Iterator>
static auto make_thrust_range(Iterator first, Iterator second) {
  return ThrustRange<Iterator>(first, second);
}

/*
 * Factory functions for creating Thrust vectors and matrices of various
 * types.
 */

template<class T>
static auto make_thrust_vector(T* x, const int n, const int incx) {
  auto begin = thrust::make_transform_iterator(
    thrust::make_counting_iterator(0),
    VectorElementFunctor<T>(x, incx));
  return make_thrust_range(begin, begin + n);
}

template<class T>
static auto make_thrust_matrix(T* A, const int m, const int n,
    const int ldA) {
  auto begin = thrust::make_transform_iterator(
    thrust::make_counting_iterator(0),
    MatrixElementFunctor<T>(A, m, ldA));
  return make_thrust_range(begin, begin + m*n);
}

template<class T>
static auto make_thrust_matrix_transpose(T* A, const int m, const int n,
    const int ldA) {
  auto begin = thrust::make_transform_iterator(
    thrust::make_counting_iterator(0),
    MatrixTransposeElementFunctor<T>(A, m, ldA));
  return make_thrust_range(begin, begin + m*n);
}

template<class T>
static auto make_thrust_matrix_lower(const T* A, const int m, const int n,
    const int ldA) {
  auto begin = thrust::make_transform_iterator(
    thrust::make_counting_iterator(0),
    MatrixLowerElementFunctor<T>(A, m, ldA));
  return make_thrust_range(begin, begin + m*n);
}

template<class T>
static auto make_thrust_matrix_symmetric(const T* A, const int m, const int n,
    const int ldA) {
  auto begin = thrust::make_transform_iterator(
    thrust::make_counting_iterator(0),
    MatrixSymmetricElementFunctor<T>(A, m, ldA));
  return make_thrust_range(begin, begin + m*n);
}

template<class T = double>
static auto make_thrust_matrix_identity(const int m, const int n) {
  auto begin = thrust::make_transform_iterator(
    thrust::make_counting_iterator(0),
    MatrixIdentityElementFunctor<T>(m));
  return make_thrust_range(begin, begin + m*n);
}

/*
 * NumBirch implementation.
 */

void numbirch::init() {
  CUBLAS_CHECK(cublasCreate(&cublasHandle));
  CUBLAS_CHECK(cublasSetStream(cublasHandle, stream));

  CUSOLVER_CHECK(cusolverDnCreate(&cusolverDnHandle));
  CUSOLVER_CHECK(cusolverDnCreateParams(&cusolverDnParams));
  CUSOLVER_CHECK(cusolverDnSetStream(cusolverDnHandle, stream));
}

void term() {
  CUSOLVER_CHECK(cusolverDnDestroyParams(cusolverDnParams));
  CUSOLVER_CHECK(cusolverDnDestroy(cusolverDnHandle));
  CUBLAS_CHECK(cublasDestroy(cublasHandle));
}

void* numbirch::malloc(const size_t size) {
  void* ptr = nullptr;
  CUDA_CHECK(cudaMallocAsync(&ptr, size, stream));
  CUDA_CHECK(cudaStreamSynchronize(stream));
  return ptr;
}

void* numbirch::realloc(void* ptr, size_t oldsize, size_t newsize) {
  void* src = ptr;
  void* dst = nullptr;
  size_t n = std::min(oldsize, newsize);
  CUDA_CHECK(cudaMallocAsync(&dst, newsize, stream));
  CUDA_CHECK(cudaMemcpyAsync(dst, src, n, cudaMemcpyDefault, stream));
  CUDA_CHECK(cudaStreamSynchronize(stream));
  CUDA_CHECK(cudaFreeAsync(src, stream));
  return dst;
}

void numbirch::free(void* ptr) {
  CUDA_CHECK(cudaFreeAsync(ptr, stream));
}

void numbirch::copy(const int n, const double* x, const int incx, double* y,
    const int incy) {
  CUDA_CHECK(cudaMemcpy2DAsync(y, incy*sizeof(double), x, incx*sizeof(double),
      n*sizeof(double), 1, cudaMemcpyDefault, stream));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

void numbirch::copy(const int m, const int n, const double* A, const int ldA,
    double* B, const int ldB) {
  CUDA_CHECK(cudaMemcpy2DAsync(B, ldB*sizeof(double), A, ldA*sizeof(double),
      m*sizeof(double), n, cudaMemcpyDefault, stream));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

void numbirch::neg(const int n, const double* x, const int incx, double* y,
    const int incy) {
  auto x1 = make_thrust_vector(x, n, incx);
  auto y1 = make_thrust_vector(y, n, incy);
  thrust::transform(policy, x1.begin(), x1.end(), y1.begin(),
      thrust::negate<double>());
}

void numbirch::neg(const int m, const int n, const double* A, const int ldA,
    double* B, const int ldB) {
  auto A1 = make_thrust_matrix(A, m, n, ldA);
  auto B1 = make_thrust_matrix(B, m, n, ldB);
  thrust::transform(policy, A1.begin(), A1.end(), B1.begin(),
      thrust::negate<double>());
}

void numbirch::add(const int n, const double* x, const int incx,
    const double* y, const int incy, double* z, const int incz) {
  auto x1 = make_thrust_vector(x, n, incx);
  auto y1 = make_thrust_vector(y, n, incy);
  auto z1 = make_thrust_vector(z, n, incz);
  thrust::transform(policy, x1.begin(), x1.end(), y1.begin(), z1.begin(),
      thrust::plus<double>());
}

void numbirch::add(const int m, const int n, const double* A, const int ldA,
    const double* B, const int ldB, double* C, const int ldC) {
  auto A1 = make_thrust_matrix(A, m, n, ldA);
  auto B1 = make_thrust_matrix(B, m, n, ldB);
  auto C1 = make_thrust_matrix(C, m, n, ldC);
  thrust::transform(policy, A1.begin(), A1.end(), B1.begin(), C1.begin(),
      thrust::plus<double>());
}

void numbirch::sub(const int n, const double* x, const int incx,
    const double* y, const int incy, double* z, const int incz) {
  auto x1 = make_thrust_vector(x, n, incx);
  auto y1 = make_thrust_vector(y, n, incy);
  auto z1 = make_thrust_vector(z, n, incz);
  thrust::transform(policy, x1.begin(), x1.end(), y1.begin(), z1.begin(),
      thrust::minus<double>());
}

void numbirch::sub(const int m, const int n, const double* A, const int ldA,
    const double* B, const int ldB, double* C, const int ldC) {
  auto A1 = make_thrust_matrix(A, m, n, ldA);
  auto B1 = make_thrust_matrix(B, m, n, ldB);
  auto C1 = make_thrust_matrix(C, m, n, ldC);
  thrust::transform(policy, A1.begin(), A1.end(), B1.begin(), C1.begin(),
      thrust::minus<double>());
}

void numbirch::hadamard(const int n, const double* x, const int incx,
    const double* y, const int incy, double* z, const int incz) {
  auto x1 = make_thrust_vector(x, n, incx);
  auto y1 = make_thrust_vector(y, n, incy);
  auto z1 = make_thrust_vector(z, n, incz);
  thrust::transform(policy, x1.begin(), x1.end(), y1.begin(), z1.begin(),
      thrust::multiplies<double>());
}

void numbirch::hadamard(const int m, const int n, const double* A,
    const int ldA, const double* B, const int ldB, double* C, const int ldC) {
  auto A1 = make_thrust_matrix(A, m, n, ldA);
  auto B1 = make_thrust_matrix(B, m, n, ldB);
  auto C1 = make_thrust_matrix(C, m, n, ldC);
  thrust::transform(policy, A1.begin(), A1.end(), B1.begin(), C1.begin(),
      thrust::multiplies<double>());
}

void numbirch::div(const int n, const double* x, const int incx,
    const double y, double* z, const int incz) {
  auto x1 = make_thrust_vector(x, n, incx);
  auto z1 = make_thrust_vector(z, n, incz);
  thrust::transform(policy, x1.begin(), x1.end(), z1.begin(),
      ScalarDivideFunctor(y));
}

void numbirch::div(const int m, const int n, const double* A, const int ldA,
    const double b, double* C, const int ldC) {
  auto A1 = make_thrust_matrix(A, m, n, ldA);
  auto C1 = make_thrust_matrix(C, m, n, ldC);
  thrust::transform(policy, A1.begin(), A1.end(), C1.begin(),
      ScalarDivideFunctor(b));
}

void numbirch::mul(const int n, const double x, const double* y,
    const int incy, double* z, const int incz) {
  auto y1 = make_thrust_vector(y, n, incy);
  auto z1 = make_thrust_vector(z, n, incz);
  thrust::transform(policy, y1.begin(), y1.end(), z1.begin(),
      ScalarMultiplyFunctor(x));
}

void numbirch::mul(const int m, const int n, const double a, const double* B,
    const int ldB, double* C, const int ldC) {
  auto B1 = make_thrust_matrix(B, m, n, ldB);
  auto C1 = make_thrust_matrix(C, m, n, ldC);
  thrust::transform(policy, B1.begin(), B1.end(), C1.begin(),
      ScalarMultiplyFunctor(a));
}

void numbirch::mul(const int m, const int n, const double* A, const int ldA,
    const double* x, const int incx, double* y, const int incy) {
  double a = 1.0;
  double b = 0.0;
  CUBLAS_CHECK(cublasDgemv(cublasHandle, CUBLAS_OP_N, m, n, &a, A, ldA, x, 
      incx, &b, y, incy));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

void numbirch::mul(const int m, const int n, const int k, const double* A,
    const int ldA, const double* B, const int ldB, double* C, const int ldC) {
  double a = 1.0;
  double b = 0.0;
  CUBLAS_CHECK(cublasDgemm(cublasHandle, CUBLAS_OP_N, CUBLAS_OP_N, m, n, k,
      &a, A, ldA, B, ldB, &b, C, ldC));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

double numbirch::sum(const int n, const double* x, const int incx) {
  auto x1 = make_thrust_vector(x, n, incx);
  return thrust::reduce(policy, x1.begin(), x1.end());
}

double numbirch::sum(const int m, const int n, const double* A,
    const int ldA) {
  auto A1 = make_thrust_matrix(A, m, n, ldA);
  return thrust::reduce(policy, A1.begin(), A1.end());
}

double numbirch::dot(const int n, const double* x, const int incx,
    const double* y, const int incy) {
  double z = 0.0;
  CUBLAS_CHECK(cublasDdot(cublasHandle, n, x, incx, y, incy, &z));
  CUDA_CHECK(cudaStreamSynchronize(stream));
  return z;
}

double numbirch::frobenius(const int m, const int n, const double* A,
    const int ldA, const double* B, const int ldB) {
  auto A1 = make_thrust_matrix(A, m, n, ldA);
  auto B1 = make_thrust_matrix(B, m, n, ldB);
  return thrust::inner_product(policy, A1.begin(), A1.end(), B1.begin(), 0.0);
}

void numbirch::inner(const int m, const int n, const double* A, const int ldA,
    const double* x, const int incx, double* y, const int incy) {
  double a = 1.0;
  double b = 0.0;
  CUBLAS_CHECK(cublasDgemv(cublasHandle, CUBLAS_OP_T, n, m, &a, A, ldA, x,
      incx, &b, y, incy));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

void numbirch::inner(const int m, const int n, const int k, const double* A,
    const int ldA, const double* B, const int ldB, double* C, const int ldC) {
  double a = 1.0;
  double b = 0.0;
  CUBLAS_CHECK(cublasDgemm(cublasHandle, CUBLAS_OP_T, CUBLAS_OP_N, m, n, k,
      &a, A, ldA, B, ldB, &b, C, ldC));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

void numbirch::outer(const int m, const int n, const double* x,
    const int incx, const double* y, const int incy, double* A,
    const int ldA) {
  /* here, the two vectors are interpreted as single-row matrices, so that the
   * stride between elements becomes the stride between columns; to create the
   * outer product, the first matrix is transposed to a single-column matrix,
   * while the second is not */
  double a = 1.0;
  double b = 0.0;
  CUBLAS_CHECK(cublasDgemm(cublasHandle, CUBLAS_OP_T, CUBLAS_OP_N, m, n, 1,
      &a, x, incx, y, incy, &b, A, ldA));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

void numbirch::outer(const int m, const int n, const int k, const double* A,
    const int ldA, const double* B, const int ldB, double* C, const int ldC) {
  double a = 1.0;
  double b = 0.0;
  CUBLAS_CHECK(cublasDgemm(cublasHandle, CUBLAS_OP_N, CUBLAS_OP_T, m, n, k,
      &a, A, ldA, B, ldB, &b, C, ldC));
  CUDA_CHECK(cudaStreamSynchronize(stream));
}

void numbirch::solve(const int n, const double* A, const int ldA, double* x,
    const int incx, const double* y, const int incy) {
  int64_t* ipiv = nullptr;
  double* LU = nullptr;
  int ldLU = n;
  int info = 0;
  double* x1 = x;

  CUDA_CHECK(cudaMallocAsync(&ipiv, sizeof(int64_t)*std::max(1, n), stream));
  CUDA_CHECK(cudaMallocAsync(&LU, sizeof(double)*std::max(1, n*n), stream));
  if (incx > 1) {
    CUDA_CHECK(cudaMallocAsync(&x1, sizeof(double)*n, stream));
  }
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXgetrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, n, n, CUDA_R_64F, LU, ldLU, CUDA_R_64F,
      &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* solve via LU factorization with partial pivoting */
  CUDA_CHECK(cudaMemcpy2DAsync(LU, ldLU*sizeof(double), A, ldA*sizeof(double),
    n*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXgetrf(cusolverDnHandle, cusolverDnParams, n, n,
      CUDA_R_64F, LU, ldLU, ipiv, CUDA_R_64F, bufferOnDevice,
      bufferOnDeviceSize, bufferOnHost, bufferOnHostSize, &info));
  CUBLAS_CHECK(cublasDcopy(cublasHandle, n, y, incy, x1, 1));
  CUSOLVER_CHECK(cusolverDnXgetrs(cusolverDnHandle, cusolverDnParams,
      CUBLAS_OP_N, n, 1, CUDA_R_64F, LU, ldLU, ipiv, CUDA_R_64F, x1, n,
      &info));
  if (incx > 1) {
    CUBLAS_CHECK(cublasDcopy(cublasHandle, n, x1, 1, x, incx));
  }
  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  if (incx > 1) {
    CUDA_CHECK(cudaFreeAsync(x1, stream));
  }
  CUDA_CHECK(cudaFreeAsync(LU, stream));
  CUDA_CHECK(cudaFreeAsync(ipiv, stream));
}

void numbirch::solve(const int m, const int n, const double* A, const int ldA,
    double* X, const int ldX, const double* Y, const int ldY) {
  int64_t *ipiv = nullptr;
  double* LU = nullptr;
  int ldLU = m;
  int info = 0;

  CUDA_CHECK(cudaMallocAsync(&ipiv, sizeof(int64_t)*std::min(m, n), stream));
  CUDA_CHECK(cudaMallocAsync(&LU, sizeof(double)*std::max(1, m*m), stream));
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXgetrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, m, m, CUDA_R_64F, LU, ldLU, CUDA_R_64F,
      &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* solve via LU factorization with partial pivoting */
  CUDA_CHECK(cudaMemcpy2DAsync(LU, ldLU*sizeof(double), A, ldA*sizeof(double),
    n*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXgetrf(cusolverDnHandle, cusolverDnParams, n, n,
      CUDA_R_64F, LU, ldLU, ipiv, CUDA_R_64F, bufferOnDevice,
      bufferOnDeviceSize, bufferOnHost, bufferOnHostSize, &info));
  CUDA_CHECK(cudaMemcpy2DAsync(X, ldX*sizeof(double), Y, ldX*sizeof(double),
    m*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXgetrs(cusolverDnHandle, cusolverDnParams,
      CUBLAS_OP_N, m, n, CUDA_R_64F, LU, ldLU, ipiv, CUDA_R_64F, X, ldX,
      &info));
  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  CUDA_CHECK(cudaFreeAsync(LU, stream));
  CUDA_CHECK(cudaFreeAsync(ipiv, stream));
}

void numbirch::cholsolve(const int n, const double* S, const int ldS,
    double* x, const int incx, const double* y, const int incy) {
  double* LLT = nullptr;
  int ldLLT = n;
  int info = 0;
  double* x1 = x;

  CUDA_CHECK(cudaMallocAsync(&LLT, sizeof(double)*std::max(1, n*n), stream));
  if (incx > 1) {
    CUDA_CHECK(cudaMallocAsync(&x1, sizeof(double)*n, stream));
  }
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXpotrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, LLT, ldLLT,
      CUDA_R_64F, &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* solve via Cholesky factorization */
  CUDA_CHECK(cudaMemcpy2DAsync(LLT, ldLLT*sizeof(double), S,
      ldS*sizeof(double), n*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXpotrf(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, LLT, ldLLT, CUDA_R_64F,
      bufferOnDevice, bufferOnDeviceSize, bufferOnHost, bufferOnHostSize,
      &info));
  CUBLAS_CHECK(cublasDcopy(cublasHandle, n, y, incy, x1, 1));
  CUSOLVER_CHECK(cusolverDnXpotrs(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, n, 1, CUDA_R_64F, LLT, ldLLT, CUDA_R_64F, x1,
      n, &info));
  if (incx > 1) {
    CUBLAS_CHECK(cublasDcopy(cublasHandle, n, x1, 1, x, incx));
  }
  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  if (incx > 1) {
    CUDA_CHECK(cudaFreeAsync(x1, stream));
  }
  CUDA_CHECK(cudaFreeAsync(LLT, stream));
}

void numbirch::cholsolve(const int m, const int n, const double* S,
    const int ldS, double* X, const int ldX, const double* Y, const int ldY) {
  double* LLT = nullptr;
  int ldLLT = m;
  int info = 0;

  CUDA_CHECK(cudaMallocAsync(&LLT, sizeof(double)*std::max(1, m*m), stream));
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXpotrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, CUBLAS_FILL_MODE_LOWER, m, CUDA_R_64F, LLT, ldLLT,
      CUDA_R_64F, &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* solve via Cholesky factorization */
  CUDA_CHECK(cudaMemcpy2DAsync(LLT, ldLLT*sizeof(double), S,
      ldS*sizeof(double), m*sizeof(double), m, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXpotrf(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, m, CUDA_R_64F, LLT, ldLLT, CUDA_R_64F,
      bufferOnDevice, bufferOnDeviceSize, bufferOnHost, bufferOnHostSize,
      &info));
  CUDA_CHECK(cudaMemcpy2DAsync(X, ldX*sizeof(double), Y, ldX*sizeof(double),
    m*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXpotrs(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, m, n, CUDA_R_64F, LLT, ldLLT, CUDA_R_64F, X,
      ldX, &info));
  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  CUDA_CHECK(cudaFreeAsync(LLT, stream));
}

void numbirch::inv(const int n, const double* A, const int ldA, double* B,
    const int ldB) {
  int64_t* ipiv = nullptr;
  double* LU = nullptr;
  int ldLU = n;
  int info = 0;

  CUDA_CHECK(cudaMallocAsync(&ipiv, sizeof(int64_t)*n, stream));
  CUDA_CHECK(cudaMallocAsync(&LU, sizeof(double)*std::max(1, n*n), stream));
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXgetrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, n, n, CUDA_R_64F, LU, ldLU, CUDA_R_64F,
      &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* solve via LU factorization with partial pivoting */
  CUDA_CHECK(cudaMemcpy2DAsync(LU, ldLU*sizeof(double), A, ldA*sizeof(double),
      n*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXgetrf(cusolverDnHandle, cusolverDnParams, n, n,
      CUDA_R_64F, LU, ldLU, ipiv, CUDA_R_64F, bufferOnDevice,
      bufferOnDeviceSize, bufferOnHost, bufferOnHostSize, &info));
  auto B1 = make_thrust_matrix(B, n, n, ldB);
  auto I = make_thrust_matrix_identity(n, n);
  thrust::async::copy(policy, I.begin(), I.end(), B1.begin());
  CUSOLVER_CHECK(cusolverDnXgetrs(cusolverDnHandle, cusolverDnParams,
      CUBLAS_OP_N, n, n, CUDA_R_64F, LU, ldLU, ipiv, CUDA_R_64F, B, ldB,
      &info));
  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  CUDA_CHECK(cudaFreeAsync(LU, stream));
  CUDA_CHECK(cudaFreeAsync(ipiv, stream));
}

void numbirch::cholinv(const int n, const double* S, const int ldS, double* B,
    const int ldB) {
  double* LLT = nullptr;
  int ldLLT = n;
  int info = 0;

  CUDA_CHECK(cudaMallocAsync(&LLT, sizeof(double)*std::max(1, n*n), stream));
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXpotrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, LLT, ldLLT,
      CUDA_R_64F, &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* solve via Cholesky factorization */
  CUDA_CHECK(cudaMemcpy2DAsync(LLT, ldLLT*sizeof(double), S,
      ldS*sizeof(double), n*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXpotrf(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, LLT, ldLLT, CUDA_R_64F,
      bufferOnDevice, bufferOnDeviceSize, bufferOnHost, bufferOnHostSize,
      &info));
  auto B1 = make_thrust_matrix(B, n, n, ldB);
  auto I = make_thrust_matrix_identity(n, n);
  thrust::async::copy(policy, I.begin(), I.end(), B1.begin());
  CUSOLVER_CHECK(cusolverDnXpotrs(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, n, n, CUDA_R_64F, LLT, ldLLT, CUDA_R_64F, B,
      ldB, &info));
  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  CUDA_CHECK(cudaFreeAsync(LLT, stream));
}

double numbirch::ldet(const int n, const double* A, const int ldA) {
  int64_t *ipiv = nullptr;
  double* LU = nullptr;
  int ldLU = n;
  int info = 0;

  CUDA_CHECK(cudaMallocAsync(&ipiv, sizeof(int64_t)*n, stream));
  CUDA_CHECK(cudaMallocAsync(&LU, sizeof(double)*std::max(1, n*n), stream));
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXgetrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, n, n, CUDA_R_64F, LU, ldLU, CUDA_R_64F,
      &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* LU factorization with partial pivoting */
  CUDA_CHECK(cudaMemcpy2DAsync(LU, ldLU*sizeof(double), A, ldA*sizeof(double),
      n*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXgetrf(cusolverDnHandle, cusolverDnParams, n, n,
      CUDA_R_64F, LU, ldLU, ipiv, CUDA_R_64F, bufferOnDevice,
      bufferOnDeviceSize, bufferOnHost, bufferOnHostSize, &info));

  /* the LU factorization is with partial pivoting, which means $|A| = (-1)^p
   * |L||U|$, where $p$ is the number of row exchanges in `ipiv`; however,
   * we're taking the logarithm of its absolute value, so can ignore the first
   * term, and the second term is just 1 as $L$ has a unit diagonal; just need
   * $|U|$ here; the logarithm of its absolute value is just the sum of the
   * logarithms of the absolute values of elements on the main diagonal */
  auto d = make_thrust_vector(LU, n, ldLU + 1);  // diagonal of LU
  double ldet = thrust::transform_reduce(policy, d.begin(), d.end(),
      LogAbsFunctor<double>(), 0.0, thrust::plus<double>());

  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  CUDA_CHECK(cudaFreeAsync(LU, stream));
  CUDA_CHECK(cudaFreeAsync(ipiv, stream));

  return ldet;
}

double numbirch::lcholdet(const int n, const double* S, const int ldS) {
  double* LLT = nullptr;
  int ldLLT = n;
  int info = 0;

  CUDA_CHECK(cudaMallocAsync(&LLT, sizeof(double)*std::max(1, n*n), stream));
  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXpotrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, LLT, ldLLT,
      CUDA_R_64F, &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* solve via Cholesky factorization */
  CUDA_CHECK(cudaMemcpy2DAsync(LLT, ldLLT*sizeof(double), S,
      ldS*sizeof(double), n*sizeof(double), n, cudaMemcpyDefault, stream));
  CUSOLVER_CHECK(cusolverDnXpotrf(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, LLT, ldLLT, CUDA_R_64F,
      bufferOnDevice, bufferOnDeviceSize, bufferOnHost, bufferOnHostSize,
      &info));

  /* log-determinant is twice the sum of logarithms of elements on the main
   * diagonal, all of which should be positive */
  auto d = make_thrust_vector(LLT, n, ldLLT + 1);  // diagonal of LLT
  double ldet = 2.0*thrust::transform_reduce(policy, d.begin(), d.end(),
      LogFunctor<double>(), 0.0, thrust::plus<double>());

  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
  CUDA_CHECK(cudaFreeAsync(LLT, stream));

  return ldet;
}

void numbirch::chol(const int n, const double* S, const int ldS, double* L,
    const int ldL) {
  int info = 0;

  size_t bufferOnDeviceSize = 0, bufferOnHostSize = 0;
  CUSOLVER_CHECK(cusolverDnXpotrf_bufferSize(cusolverDnHandle,
      cusolverDnParams, CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, L, ldL,
      CUDA_R_64F, &bufferOnDeviceSize, &bufferOnHostSize));
  void *bufferOnDevice = nullptr, *bufferOnHost = nullptr;
  CUDA_CHECK(cudaMallocAsync(&bufferOnDevice,
      sizeof(double)*bufferOnDeviceSize, stream));
  CUDA_CHECK(cudaMallocAsync(&bufferOnHost,
      sizeof(double)*bufferOnHostSize, stream));

  /* copy lower triangle, and zero upper triangle */
  auto S1 = make_thrust_matrix_lower(S, n, n, ldS);
  auto L1 = make_thrust_matrix(L, n, n, ldL);
  thrust::async::copy(policy, S1.begin(), S1.end(), L1.begin());
  CUSOLVER_CHECK(cusolverDnXpotrf(cusolverDnHandle, cusolverDnParams,
      CUBLAS_FILL_MODE_LOWER, n, CUDA_R_64F, L, ldL, CUDA_R_64F,
      bufferOnDevice, bufferOnDeviceSize, bufferOnHost, bufferOnHostSize,
      &info));

  CUDA_CHECK(cudaStreamSynchronize(stream));
  assert(info == 0);

  CUDA_CHECK(cudaFreeAsync(bufferOnHost, stream));
  CUDA_CHECK(cudaFreeAsync(bufferOnDevice, stream));
}

void numbirch::transpose(const int m, const int n, const double x,
    const double* A, const int ldA, double* B, const int ldB) {
  auto A1 = make_thrust_matrix_transpose(A, m, n, ldA);
  auto B1 = make_thrust_matrix(B, m, n, ldB);
  thrust::transform(policy, A1.begin(), A1.end(), B1.begin(),
      ScalarMultiplyFunctor(x));
}

double numbirch::trace(const int m, const int n, const double* A,
    const int ldA) {
  return sum(std::min(m, n), A, ldA + 1);
}
