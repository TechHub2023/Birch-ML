/**
 * @file
 */
#pragma once

#include "numbirch/utility.hpp"

#include <type_traits>
#include <cstdint>

#ifdef __CUDACC__
#define NUMBIRCH_HOST __host__
#else
#define NUMBIRCH_HOST
#endif

#ifdef __CUDACC__
#define NUMBIRCH_DEVICE __device__
#else
#define NUMBIRCH_DEVICE
#endif

#ifdef __CUDACC__
#define NUMBIRCH_HOST_DEVICE __host__ __device__
#else
#define NUMBIRCH_HOST_DEVICE
#endif

#ifdef __CUDACC__
#define NUMBIRCH_CONSTANT __constant__
#else
#define NUMBIRCH_CONSTANT
#endif

/**
 * @internal
 * 
 * @def ARRAY
 * 
 * Constructs the type `Array<T,D>`.
 */
#define NUMBIRCH_ARRAY(T, D) Array<T,D>

/**
 * @internal
 * 
 * @def NUMBIRCH_REAL
 * 
 * Macro to set the default floating point type. Valid values are `float` and
 * `double`.
 */
#ifndef NUMBIRCH_REAL
#define NUMBIRCH_REAL double
#endif

namespace numbirch {
template<class T, int D> class Array;

/**
 * Default floating point type. This is set to the value of the macro
 * `NUMBIRCH_REAL`, or `double` if undefined.
 * 
 * @ingroup trait
 */
using real = NUMBIRCH_REAL;

/**
 * @internal
 * 
 * @def PI
 * 
 * Value of pi.
 */
static const real PI = 3.1415926535897932384626433832795;

/**
 * @var is_bool_v
 * 
 * Is `T` type `bool`?
 * 
 * @ingroup trait
 */
template<class T>
struct is_bool {
  static constexpr bool value = false;
};
template<>
struct is_bool<bool> {
  static constexpr bool value = true;
};
template<class T>
inline constexpr bool is_bool_v = is_bool<std::decay_t<T>>::value;

/**
 * @var is_int_v
 * 
 * Is `T` type `int`?
 * 
 * @ingroup trait
 * 
 * @see c.f. [std::is_integral]
 * (https://en.cppreference.com/w/cpp/types/is_int)
 */
template<class T>
struct is_int {
  static constexpr bool value = false;
};
template<>
struct is_int<int> {
  static constexpr bool value = true;
};
template<class T>
inline constexpr bool is_int_v = is_int<std::decay_t<T>>::value;

/**
 * @var is_real_v
 * 
 * Is `T` a floating point type?
 * 
 * @ingroup trait
 * 
 * The only floating point type is `real`, which is an alias of either
 * `double` or `float` according to the NumBirch build.
 * 
 * @see c.f. [std::is_floating_point]
 * (https://en.cppreference.com/w/cpp/types/is_floating_point)
 */
template<class T>
struct is_real {
  static constexpr bool value = false;
};
template<>
struct is_real<real> {
  static constexpr bool value = true;
};
template<class T>
inline constexpr bool is_real_v = is_real<std::decay_t<T>>::value;

/**
 * @var is_arithmetic_v
 * 
 * Is `T` an arithmetic type?
 * 
 * @ingroup trait
 * 
 * An arithmetic type is one of `bool`, `int`, or `real`.
 * 
 * @see is_bool, is_int, is_real, c.f. [std::is_arithmetic]
 * (https://en.cppreference.com/w/cpp/types/is_arithmetic)
 */
template<class T>
struct is_arithmetic {
  static constexpr bool value = is_bool<T>::value || is_int<T>::value ||
      is_real<T>::value;
};
template<class T>
inline constexpr bool is_arithmetic_v = is_arithmetic<std::decay_t<T>>::value;

/**
 * @typedef value_t
 *
 * Value type of an arithmetic type. For a basic type this is an identity
 * function, for an array type it is the element type.
 * 
 * @ingroup trait
 */
template<class T>
struct value_s {
  using type = T;
};
template<class T, int D>
struct value_s<Array<T,D>> {
  using type = T;
};
template<class T>
using value_t = typename value_s<std::decay_t<T>>::type;

/**
 * @var dimension_v
 *
 * Dimension of an arithmetic type.
 * 
 * @ingroup trait
 */
template<class T>
struct dimension {
  static constexpr int value = 0;
};
template<class T, int D>
struct dimension<Array<T,D>> {
  static constexpr int value = D;
};
template<class T>
inline constexpr int dimension_v = dimension<std::decay_t<T>>::value;

/**
 * @var is_array_v
 * 
 * Is `T` an array type?
 * 
 * @ingroup trait
 * 
 * An array type is any instantiation of Array, including one with zero
 * dimensions.
 */
template<class T>
struct is_array {
  static constexpr bool value = false;
};
template<class T, int D>
struct is_array<Array<T,D>> {
  static constexpr bool value = true;
};
template<class T>
inline constexpr bool is_array_v = is_array<std::decay_t<T>>::value;

/**
 * @var is_scalar_v
 * 
 * Is `T` a scalar type?
 * 
 * @ingroup trait
 * 
 * A scalar type is any numeric type with zero dimensions.
 */
template<class T>
struct is_scalar {
  static constexpr bool value = is_arithmetic_v<value_t<T>> &&
      dimension<T>::value == 0;
};
template<class T>
inline constexpr bool is_scalar_v = is_scalar<std::decay_t<T>>::value;

/**
 * @var is_vector_v
 * 
 * Is `T` a vector type?
 * 
 * @ingroup trait
 * 
 * A vector type is any numeric type with one dimension.
 */
template<class T>
struct is_vector {
  static constexpr bool value = is_arithmetic_v<value_t<T>> &&
      dimension<T>::value == 1;
};
template<class T>
inline constexpr bool is_vector_v = is_vector<std::decay_t<T>>::value;

/**
 * @var is_matrix_v
 * 
 * Is `T` a matrix type?
 * 
 * @ingroup trait
 * 
 * A matrix type is any numeric type with two dimensions.
 */
template<class T>
struct is_matrix {
  static constexpr bool value = is_arithmetic_v<value_t<T>> &&
      dimension<T>::value == 2;
};
template<class T>
inline constexpr bool is_matrix_v = is_matrix<std::decay_t<T>>::value;

/**
 * @var is_numeric_v
 * 
 * Is `T` a numeric type?
 * 
 * @ingroup trait
 * 
 * An numeric type is an array or scalar type.
 * 
 * @see is_array, is_scalar
 */
template<class T>
struct is_numeric {
  static constexpr bool value = is_array<T>::value || is_scalar<T>::value;
};
template<class T>
inline constexpr bool is_numeric_v = is_numeric<std::decay_t<T>>::value;

/**
 * @typedef promote_t
 * 
 * Promoted arithmetic type for a collection of arithmetic types.
 * 
 * @tparam Args Arithmetic types.
 * 
 * Gives the type, among that collection, that is highest in the promotion
 * order (`bool` to `int` to `float` to `double`).
 * 
 * @ingroup trait
 */
template<class... Args>
struct promote {
  using type = void;
};
template<class T, class U, class... Args>
struct promote<T,U,Args...> {
  using type = typename promote<typename promote<std::decay_t<T>,
      std::decay_t<U>>::type,Args...>::type;
};
template<>
struct promote<double,double> {
  using type = double;
};
template<>
struct promote<double,float> {
  using type = double;
};
template<>
struct promote<double,int> {
  using type = double;
};
template<>
struct promote<double,bool> {
  using type = double;
};
template<>
struct promote<float,double> {
  using type = double;
};
template<>
struct promote<float,float> {
  using type = float;
};
template<>
struct promote<float,int> {
  using type = float;
};
template<>
struct promote<float,bool> {
  using type = float;
};
template<>
struct promote<int,double> {
  using type = double;
};
template<>
struct promote<int,float> {
  using type = float;
};
template<>
struct promote<int,int> {
  using type = int;
};
template<>
struct promote<int,bool> {
  using type = int;
};
template<>
struct promote<bool,double> {
  using type = double;
};
template<>
struct promote<bool,float> {
  using type = float;
};
template<>
struct promote<bool,int> {
  using type = int;
};
template<>
struct promote<bool,bool> {
  using type = bool;
};
template<class T, class U>
struct promote<T,U> {
  using type = void;
};
template<class T>
struct promote<T> {
  using type = T;
};
template<class... Args>
using promote_t = typename promote<Args...>::type;

/**
 * @typedef implicit_t
 * 
 * Implicit return type.
 * 
 * @tparam Args Numeric types.
 * 
 * For arithmetic types this works as promote_t. If one or more of the
 * numeric types is an array type, then gives an array type `Array<T,D>`
 * where `T` is the promotion of all the element types among all arguments,
 * and `D` the largest number of dimensions among all arguments.
 * 
 * @ingroup trait
 */
template<class... Args>
struct implicit {
  using type = void;
};
template<class T, class U, class... Args>
struct implicit<T,U,Args...> {
  using type = typename implicit<typename implicit<std::decay_t<T>,
      std::decay_t<U>>::type,Args...>::type;
};
template<class T, int D, class U, int E>
struct implicit<Array<T,D>,Array<U,E>> {
  using type = void;
};
template<class T, int D, class U>
struct implicit<Array<T,D>,Array<U,D>> {
  using type = Array<typename promote<T,U>::type,D>;
};
template<class T, int D, class U>
struct implicit<Array<T,D>,Array<U,0>> {
  using type = Array<typename promote<T,U>::type,D>;
};
template<class T, int D, class U>
struct implicit<Array<T,0>,Array<U,D>> {
  using type = Array<typename promote<T,U>::type,D>;
};
template<class T, class U>
struct implicit<Array<T,0>,Array<U,0>> {
  using type = Array<typename promote<T,U>::type,0>;
};
template<class T, int D, class U>
struct implicit<Array<T,D>,U> {
  using type = Array<typename promote<T,U>::type,D>;
};
template<class T, class U, int D>
struct implicit<T,Array<U,D>> {
  using type = Array<typename promote<T,U>::type,D>;
};
template<class T, class U>
struct implicit<T,U> {
  using type = typename promote<T,U>::type;
};
template<class T>
struct implicit<T> {
  using type = T;
};
template<class... Args>
using implicit_t = typename implicit<Args...>::type;

/**
 * @typedef explicit_t
 * 
 * Explicit override of return type.
 * 
 * @tparam R Arithmetic type.
 * @tparam Args Numeric types.
 * 
 * This works as for implicit_t, but overrides the element type with `R`.
 * 
 * @ingroup trait
 */
template<class... Args>
struct explicit_s {
  using type = void;
};
template<class R, class... Args>
struct explicit_s<R,Args...> {
  using type = typename explicit_s<R,typename implicit<Args...>::type>::type;
};
template<class R, class T, int D>
struct explicit_s<R,Array<T,D>> {
  using type = Array<R,D>;
};
template<class R, class T>
struct explicit_s<R,T> {
  using type = R;
};
template<class R, class... Args>
using explicit_t = typename explicit_s<R,Args...>::type;

/**
 * @typedef real_t
 * 
 * @ingroup trait
 * 
 * Floating point override of return type.
 * 
 * @tparam Args Numeric types.
 * 
 * This works as for implicit_t, but overrides the element type with `real`;
 * equivalently `explicit_t<real,Args...>`.
 * 
 * @ingroup trait
 */
template<class... Args>
struct real_s {
  using type = typename explicit_s<real,Args...>::type;
};
template<class... Args>
using real_t = typename real_s<Args...>::type;

/**
 * @typedef int_t
 * 
 * @ingroup trait
 * 
 * Integer override of return type.
 * 
 * @tparam Args Numeric types.
 * 
 * This works as for implicit_t, but overrides the element type with `int`;
 * equivalently `explicit_t<int,Args...>`.
 * 
 * @ingroup trait
 */
template<class... Args>
struct int_s {
  using type = typename explicit_s<int,Args...>::type;
};
template<class... Args>
using int_t = typename int_s<Args...>::type;

/**
 * @typedef bool_t
 * 
 * @ingroup trait
 * 
 * Boolean override of return type.
 * 
 * @tparam Args Numeric types.
 * 
 * This works as for implicit_t, but overrides the element type with `bool`;
 * equivalently `explicit_t<bool,Args...>`.
 * 
 * @ingroup trait
 */
template<class... Args>
struct bool_s {
  using type = typename explicit_s<bool,Args...>::type;
};
template<class... Args>
using bool_t = typename bool_s<Args...>::type;

/**
 * @internal
 * 
 * Does arithmetic type `T` promote to `U` under promotion rules?
 * 
 * @ingroup trait
 * 
 * @tparam T Arithmetic type.
 * @tparam U Arithmetic type.
 */
template<class T, class U>
struct promotes_to {
  static constexpr bool value = std::is_same<
      promote_t<T,U>,std::decay_t<U>>::value;
};
template<class T, class U>
inline constexpr bool promotes_to_v = promotes_to<T,U>::value;

/**
 * @var all_integral_v
 * 
 * Are all argument types integral?
 * 
 * @ingroup trait
 */
template<class... Args>
struct all_integral {
  //
};
template<class Arg>
struct all_integral<Arg> {
  static const bool value = is_int<Arg>::value;
};
template<class Arg, class... Args>
struct all_integral<Arg,Args...> {
  static const bool value = is_int<Arg>::value &&
      all_integral<Args...>::value;
};
template<class... Args>
inline constexpr bool all_integral_v = all_integral<Args...>::value;

/**
 * @typedef pack_t
 * 
 * @ingroup trait
 * 
 * Return type of pack().
 */
template<class T, class U>
using pack_t = Array<promote_t<value_t<T>,value_t<U>>,2>;

/**
 * @typedef stack_t
 * 
 * @ingroup trait
 * 
 * Return type of stack().
 */
template<class T, class U>
using stack_t = Array<promote_t<value_t<T>,value_t<U>>,
    (dimension_v<T> == 2 || dimension_v<T> == 2) ? 2 : 1>;

/**
 * @internal
 *
 * 0-based element of a matrix, vector, or scalar. A scalar is identified by
 * having `ld == 0`.
 */
template<class T>
NUMBIRCH_HOST_DEVICE T& get(T* x, const int i = 0, const int j = 0,
    const int ld = 0) {
  int64_t k = (ld == 0) ? 0 : (i + j*int64_t(ld));
  return x[k];
}

/**
 * @internal
 *
 * 0-based element of a matrix, vector, or scalar. A scalar is identified by
 * having `ld == 0`.
 */
template<class T>
NUMBIRCH_HOST_DEVICE const T& get(const T* x, const int i = 0,
    const int j = 0, const int ld = 0) {
  int64_t k = (ld == 0) ? 0 : (i + j*int64_t(ld));
  return x[k];
}

/**
 * @internal
 * 
 * 0-based element of a scalar---just returns the scalar.
 */
template<class T, class = std::enable_if_t<is_arithmetic_v<T>,int>>
NUMBIRCH_HOST_DEVICE T& get(T& x, const int i = 0, const int j = 0,
    const int ld = 0) {
  return x;
}

/**
 * @internal
 * 
 * 0-based element of a scalar---just returns the scalar.
 */
template<class T, class = std::enable_if_t<is_arithmetic_v<T>,int>>
NUMBIRCH_HOST_DEVICE const T& get(const T& x, const int i = 0,
    const int j = 0, const int ld = 0) {
  return x;
}

/**
 * @internal
 * 
 * Performs the inverse operation of a scalar broadcast during gradient
 * computation. That is, if a scalar was broadcast during the forward pass,
 * upstream gradients must be aggregated, by summation, during the backward
 * pass.
 */
template<int D, class T>
constexpr auto aggregate(const T& x) {
  if constexpr (D == 0 && dimension_v<T> != 0) {
    return sum(x);
  } else {
    return x;
  }
}

/**
 * @internal
 * 
 * Value of a scalar.
 */
template<class T>
constexpr auto value(const Array<T,0>& y) {
  return y.value();
}

/**
 * @internal
 * 
 * Value of a scalar.
 */
template<class T, class = std::enable_if_t<is_arithmetic_v<T>,int>>
constexpr auto value(const T& y) {
  return y;
}

}
