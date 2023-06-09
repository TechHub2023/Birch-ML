/**
 * @file
 */
#include "numbirch/cuda/cublas.hpp"

#if HAVE_OMP_H
#include <omp.h>
#endif

namespace numbirch {

thread_local cublasHandle_t cublasHandle;
thread_local void* cublasWorkspace;

double* oneD = nullptr;
double* zeroD = nullptr;
float* oneS = nullptr;
float* zeroS = nullptr;
int* oneI = nullptr;
int* zeroI = nullptr;

void cublas_init() {
  /* double-precision scalars */
  double oneD1 = 1.0;
  double zeroD1 = 0.0;
  CUDA_CHECK(cudaMalloc(&oneD, sizeof(double)));
  CUDA_CHECK(cudaMalloc(&zeroD, sizeof(double)));  
  CUBLAS_CHECK(cublasSetVector(1, sizeof(double), &oneD1, 1, oneD, 1));
  CUBLAS_CHECK(cublasSetVector(1, sizeof(double), &zeroD1, 1, zeroD, 1));

  /* single-precision scalars */
  float oneS1 = 1.0f;
  float zeroS1 = 0.0f;
  CUDA_CHECK(cudaMalloc(&oneS, sizeof(float)));
  CUDA_CHECK(cudaMalloc(&zeroS, sizeof(float)));  
  CUBLAS_CHECK(cublasSetVector(1, sizeof(float), &oneS1, 1, oneS, 1));
  CUBLAS_CHECK(cublasSetVector(1, sizeof(float), &zeroS1, 1, zeroS, 1));

  /* integer scalars */
  int oneI1 = 1;
  int zeroI1 = 0;
  CUDA_CHECK(cudaMalloc(&oneI, sizeof(int)));
  CUDA_CHECK(cudaMalloc(&zeroI, sizeof(int)));  
  CUBLAS_CHECK(cublasSetVector(1, sizeof(int), &oneI1, 1, oneI, 1));
  CUBLAS_CHECK(cublasSetVector(1, sizeof(int), &zeroI1, 1, zeroI, 1));

  #pragma omp parallel num_threads(omp_get_max_threads())
  {
    size_t size = 1 << 21; // 4 MB as recommended in CUBLAS docs
    CUBLAS_CHECK(cublasCreate(&cublasHandle));
    CUBLAS_CHECK(cublasSetStream(cublasHandle, stream));
    CUDA_CHECK(cudaMalloc(&cublasWorkspace, size));
    CUBLAS_CHECK(cublasSetWorkspace(cublasHandle, cublasWorkspace, size));
    CUBLAS_CHECK(cublasSetPointerMode(cublasHandle,
        CUBLAS_POINTER_MODE_DEVICE));
  }

}

void cublas_term() {
  #pragma omp parallel num_threads(omp_get_max_threads())
  {
    CUBLAS_CHECK(cublasDestroy(cublasHandle));
    CUDA_CHECK(cudaFree(cublasWorkspace));
  }

  CUDA_CHECK(cudaFree(zeroD));
  CUDA_CHECK(cudaFree(oneD));
  CUDA_CHECK(cudaFree(zeroS));
  CUDA_CHECK(cudaFree(oneS));
}

}
