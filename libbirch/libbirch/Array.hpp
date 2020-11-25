/**
 * @file
 */
#pragma once

#include "libbirch/external.hpp"
#include "libbirch/memory.hpp"
#include "libbirch/type.hpp"
#include "libbirch/Shape.hpp"
#include "libbirch/Buffer.hpp"
#include "libbirch/Iterator.hpp"
#include "libbirch/Eigen.hpp"
#include "libbirch/ReadersWriterLock.hpp"

namespace libbirch {
/**
 * Array.
 *
 * @tparam T Value type.
 * @tparam F Shape type.
 */
template<class T, class F>
class Array {
  template<class U, class G> friend class Array;
public:
  using this_type = Array<T,F>;
  using value_type = T;
  using shape_type = F;
  using eigen_type = typename eigen_type<this_type>::type;
  using eigen_stride_type = typename eigen_stride_type<this_type>::type;

  /**
   * Constructor.
   */
  Array() :
      shape(),
      buffer(nullptr),
      offset(0),
      isView(false) {
    assert(shape.volume() == 0);
  }

  /**
   * Constructor.
   *
   * @param shape Shape.
   */
  Array(const F& shape) :
      shape(shape),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    initialize();
  }

  /**
   * Constructor.
   *
   * @tparam ...Args Constructor parameter types.
   *
   * @param shape Shape.
   * @param args Constructor arguments.
   */
  template<class... Args>
  Array(const F& shape, Args ... args) :
      shape(shape),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    initialize(args...);
  }

  /**
   * Constructor.
   *
   * @param shape Shape.
   * @param values Values.
   */
  template<class G = F, std::enable_if_t<G::count() == 1,int> = 0>
  Array(const std::initializer_list<T>& values) :
      shape(values.size()),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    std::uninitialized_copy(values.begin(), values.end(), begin());
  }

  /**
   * Constructor.
   *
   * @param shape Shape.
   * @param values Values.
   */
  template<class G = F, std::enable_if_t<G::count() == 2,int> = 0>
  Array(const std::initializer_list<std::initializer_list<T>>& values) :
      shape(values.size(), values.begin()->size()),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    auto ptr = buf();
    for (auto row : values) {
      for (auto value : row) {
        new (ptr++) T(value);
      }
    }
  }

  /**
   * Constructor.
   *
   * @param l Lambda called to construct each element.
   * @param shape Shape.
   */
  template<class L>
  Array(const L& l, const F& shape) :
      shape(shape),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    int64_t n = 0;
    for (auto iter = begin(); iter != end(); ++iter) {
      new (&*iter) T(l(n++));
    }
  }

  /**
   * Copy constructor. For value types, this uses a copy-on-write facility.
   */
  Array(const Array<T,F>& o) :
      shape(o.shape),
      buffer(o.buffer),
      offset(o.offset),
      isView(false) {
    if (o.buffer) {
      if (!o.isView && is_value<T>::value) {
        /* copy on write for non-views of value types */
        buffer->incUsage();
      } else {
        /* immediate copy for others */
        buffer = nullptr;
        offset = 0;
        allocate();
        uninitialized_copy(o);
      }
    }
  }

  /**
   * Generic copy constructor.
   */
  template<class U, class G, std::enable_if_t<F::count() == G::count() &&
      std::is_convertible<U,T>::value,int> = 0>
  Array(const Array<U,G>& o) :
      shape(o.shape.compact()),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    uninitialized_copy(o);
  }

  /**
   * Destructor.
   */
  ~Array() {
    release();
  }

  /**
   * Fix after a bitwise copy.
   */
  void bitwiseFix() {
    assert(!isView);
    bufferLock.bitwiseFix();
    if (buffer) {
      if (is_value<T>::value) {
        buffer->incUsage();
      } else {
        auto bytes = Buffer<T>::size(volume());
        assert(bytes > 0u);
	      void* src = buf();
        buffer = new (libbirch::allocate(bytes)) Buffer<T>();
        offset = 0;
        void* dst = buf();
        std::memcpy(dst, src, sizeof(T)*volume());
      }
    }
  }

  /**
   * Copy assignment operator.
   */
  Array<T,F>& operator=(const Array<T,F>& o) {
    return assign(o);
  }

  /**
   * Accept visitor.
   */
  template<class Visitor>
  void accept_(const Visitor& v) {
    if (!is_value<T>::value) {
      auto iter = begin();
      auto last = end();
      for (; iter != last; ++iter) {
        v.visit(*iter);
      }
    }
  }

  /**
   * Copy assignment. For a view the shapes of the two arrays must
   * conform, otherwise a resize is permitted.
   */
  Array<T,F>& assign(const Array<T,F>& o) {
    if (isView) {
      libbirch_assert_msg_(o.shape.conforms(shape), "array sizes are different");
      copy(o);
    } else {
      lock();
      if (o.isView) {
        Array<T,F> tmp(o.shape, o);
        swap(tmp);
      } else {
        Array<T,F> tmp(o);
        swap(tmp);
      }
      unlock();
    }
    return *this;
  }

  /**
   * Number of elements.
   */
  auto size() const {
    return shape.size();
  }

  /**
   * Number of elements allocated.
   */
  auto volume() const {
    return shape.volume();
  }

  /**
   * @name Element access, caller not responsible for thread safety
   */
  ///@{
  /**
   * Slice.
   *
   * @tparam V Slice type.
   *
   * @param slice Slice.
   *
   * @return The resulting view or element.
   */
  template<class V, class U, std::enable_if_t<V::rangeCount() != 0,int> = 0>
  auto set(const V& slice, const U& value) {
    pinWrite();
    Array<T,decltype(shape(slice))> o(shape(slice), buffer, offset +
        shape.serial(slice));
    o = value;
    unpin();
    return o;
  }

  template<class V, class U, std::enable_if_t<V::rangeCount() == 0,int> = 0>
  T& set(const V& slice, const U& value) {
    pinWrite();
    auto& o = (*(buf() + shape.serial(slice)) = value);
    unpin();
    return o;
  }

  template<class V, std::enable_if_t<V::rangeCount() != 0,int> = 0>
  auto get(const V& slice) const {
    return Array<T,decltype(shape(slice))>(shape(slice), buffer, offset +
        shape.serial(slice));
  }

  template<class V, std::enable_if_t<V::rangeCount() == 0,int> = 0>
  const T& get(const V& slice) const {
    return *(buf() + shape.serial(slice));
  }
  ///@}

  /**
   * @name Element access, caller responsible for thread safety
   */
  ///@{
  /**
   * Iterator pointing to the first element.
   *
   * Iterators are used to access the elements of an array sequentially.
   * Elements are visited in the order in which they are stored in memory;
   * the rightmost dimension is the fastest moving (for a matrix, this is
   * "row major" order).
   */
  Iterator<T,F> begin() {
    assert(!isShared());
    return Iterator<T,F>(buf(), shape);
  }
  Iterator<T,F> begin() const {
    return Iterator<T,F>(buf(), shape);
  }
  Iterator<T,F> end() {
    assert(!isShared());
    return begin() + size();
  }
  Iterator<T,F> end() const {
    return begin() + size();
  }

  /**
   * Slice.
   *
   * @tparam V Slice type.
   *
   * @param slice Slice.
   *
   * @return The resulting view or element.
   */
  template<class V, std::enable_if_t<V::rangeCount() != 0,int> = 0>
  auto operator()(const V& slice) {
    assert(!isShared());
    return Array<T,decltype(shape(slice))>(shape(slice),
        buffer, offset + shape.serial(slice));
  }
  template<class V, std::enable_if_t<V::rangeCount() != 0,int> = 0>
  auto operator()(const V& slice) const {
    return Array<T,decltype(shape(slice))>(shape(slice),
        buffer, offset + shape.serial(slice));
  }
  template<class V, std::enable_if_t<V::rangeCount() == 0,int> = 0>
  value_type& operator()(const V& slice) {
    assert(!isShared());
    return *(buf() + shape.serial(slice));
  }
  template<class V, std::enable_if_t<V::rangeCount() == 0,int> = 0>
  value_type operator()(const V& slice) const {
    return *(buf() + shape.serial(slice));
  }

  /**
   * Compare.
   */
  template<class U, class G>
  bool operator==(const Array<U,G>& o) const {
    pin();
    o.pin();
    auto result = std::equal(begin(), end(), o.begin());
    o.unpin();
    unpin();
    return result;
  }
  template<class U, class G>
  bool operator!=(const Array<U,G>& o) const {
    return !(*this == o);
  }

  /**
   * Pin the buffer. This prevents substitution of the buffer by
   * copy-on-write operations until unpinned.
   */
  void pin() const {
    const_cast<Array*>(this)->bufferLock.setRead();
  }

  /**
   * As pin(), but furthermore ensures that the buffer is not shared, and
   * thus its contents eligible for writing. If shared, a copy is performed.
   * This is used to perform copy-on-write (if necessary) before writing the
   * contents of the buffer.
   */
  void pinWrite() {
    assert(!isView);
    if (isShared()) {
      bufferLock.setWrite();
      if (isShared()) {
        Array<T,F> tmp(shape, *this);
        swap(tmp);
      }
      bufferLock.downgrade();
    } else {
      bufferLock.setRead();
    }
  }

  /**
   * Unpin the buffer.
   */
  void unpin() const {
    const_cast<Array*>(this)->bufferLock.unsetRead();
  }

  /**
   * Lock the buffer. This is used before substitution of the buffer by a
   * copy-on-write operation.
   */
  void lock() {
    bufferLock.setWrite();
  }

  /**
   * Unlock the buffer.
   */
  void unlock() {
    bufferLock.unsetWrite();
  }
  ///@}

  /**
   * @name Thread-safe resize
   */
  ///@{
  /**
   * For a one-dimensional array, push an element onto the end. This increases
   * the array size by one.
   * 
   * @param x Value.
   */
  void push(const T& x) {
    insert(size(), x);
  }

  /**
   * For a one-dimensional array, insert an element at a given position. This
   * increases the array size by one.
   *
   * @param i Position.
   * @param x Value.
   */
  void insert(const int64_t i, const T& x) {
    static_assert(F::count() == 1, "can only enlarge one-dimensional arrays");
    assert(!isView);

    lock();
    auto n = size();
    auto s = F(n + 1);
    if (!buffer || isShared()) {
      Array<T,F> tmp(s, *this);
      swap(tmp);
    } else {
      auto oldBytes = Buffer<T>::size(shape.volume());
      auto newBytes = Buffer<T>::size(s.volume());
      buffer = (Buffer<T>*)libbirch::reallocate(buffer, oldBytes,
          buffer->tid, newBytes);
    }
    std::memmove((void*)(buf() + i + 1), (void*)(buf() + i), (n - i)*sizeof(T));
    new (buf() + i) T(x);
    shape = s;
    unlock();
  }

  /**
   * For a one-dimensional array, erase elements from a given position. This
   * decreases the array size by the number of elements.
   *
   * @param i Position.
   * @param len Number of elements to erase.
   */
  void erase(const int64_t i, const int64_t len = 1) {
    static_assert(F::count() == 1, "can only shrink one-dimensional arrays");
    assert(!isView);
    assert(len > 0);
    assert(size() >= len);

    lock();
    auto n = size();
    auto s = F(n - len);
    if (s.size() == 0) {
      release();
    } else {
      if (isShared()) {
        Array<T,F> tmp(shape, *this);
        swap(tmp);
      }
      for (int j = i; j < i + len; ++j) {
        buf()[j].~T();
      }
      std::memmove((void*)(buf() + i), (void*)(buf() + i + len), (n - len - i)*sizeof(T));
      auto oldBytes = Buffer<T>::size(shape.volume());
      auto newBytes = Buffer<T>::size(s.volume());
      buffer = (Buffer<T>*)libbirch::reallocate(buffer, oldBytes,
          buffer->tid, newBytes);
    }
    shape = s;
    unlock();
  }
  ///@}

  /**
   * @name Eigen integration
   */
  ///@{
  template<class Check = T, std::enable_if_t<is_value<Check>::value,int> = 0>
  operator eigen_type() const {
    return toEigen();
  }

  template<class Check = T, std::enable_if_t<is_value<Check>::value,int> = 0>
  auto toEigen() {
    return eigen_type(buf(), rows(), cols(), eigen_stride_type(rowStride(),
        colStride()));
  }

  template<class Check = T, std::enable_if_t<is_value<Check>::value,int> = 0>
  auto toEigen() const {
    return eigen_type(buf(), rows(), cols(), eigen_stride_type(rowStride(),
        colStride()));
  }

  /**
   * Construct from Eigen Matrix expression.
   */
  template<class EigenType, std::enable_if_t<is_eigen_compatible<this_type,EigenType>::value,int> = 0>
  Array(const Eigen::MatrixBase<EigenType>& o) :
      shape(o.rows(), o.cols()),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    toEigen() = o;
  }

  /**
   * Construct from Eigen DiagonalWrapper expression.
   */
  template<class EigenType, std::enable_if_t<is_diagonal_compatible<this_type,EigenType>::value,int> = 0>
  Array(const Eigen::DiagonalWrapper<EigenType>& o) :
      shape(o.rows(), o.cols()),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    toEigen() = o;
  }

  /**
   * Construct from Eigen TriangularWrapper expression.
   */
  template<class EigenType, unsigned Mode, std::enable_if_t<is_triangle_compatible<this_type,EigenType>::value,int> = 0>
  Array(const Eigen::TriangularView<EigenType,Mode>& o)  :
      shape(o.rows(), o.cols()),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    toEigen() = o;
  }

  /**
   * Number of rows. For a vectoor, this is the length.
   */
  auto rows() const {
    assert(1 <= F::count() && F::count() <= 2);
    return shape.length(0);
  }

  /**
   * Number of columns. For a vector, this is 1.
   */
  auto cols() const {
    assert(1 <= F::count() && F::count() <= 2);
    return F::count() == 1 ? 1 : shape.length(1);
  }

  /**
   * Stride between rows.
   */
  auto rowStride() const {
    assert(1 <= F::count() && F::count() <= 2);
    return F::count() == 1 ? shape.volume() : shape.stride(0);
  }

  /**
   * Stride between columns.
   */
  auto colStride() const {
    assert(1 <= F::count() && F::count() <= 2);
    return F::count() == 1 ? shape.stride(0) : shape.stride(1);
  }
  ///@}

private:
  /**
   * Constructor for forced copy.
   */
  template<class U, class G>
  Array(const F& shape, const Array<U,G>& o) :
      shape(shape.compact()),
      buffer(nullptr),
      offset(0),
      isView(false) {
    allocate();
    uninitialized_copy(o);
  }

  /**
   * Constructor for views.
   */
  Array(const F& shape, Buffer<T>* buffer, int64_t offset) :
      shape(shape),
      buffer(buffer),
      offset(offset),
      isView(true) {
    //
  }

  /**
   * Raw pointer to underlying buffer.
   */
  T* buf() const {
    return buffer->buf() + offset;
  }

  /**
   * Is the buffer shared with one or more other arrays?
   */
  bool isShared() const {
    return buffer && buffer->numUsage() > 1u;
  }

  /**
   * Swap with another array.
   */
  void swap(Array<T,F>& o) {
    assert(!isView);
    assert(!o.isView);
    std::swap(buffer, o.buffer);
    std::swap(shape, o.shape);
    std::swap(offset, o.offset);
  }

  /**
   * Allocate memory for array, leaving uninitialized.
   */
  void allocate() {
    assert(!buffer);
    auto bytes = Buffer<T>::size(volume());
    if (bytes > 0u) {
      buffer = new (libbirch::allocate(bytes)) Buffer<T>();
      offset = 0;
    }
  }

  /**
   * Deallocate memory of array.
   */
  void release() {
    if (!isView && buffer && buffer->decUsage() == 0u) {
      if (!is_value<T>::value) {
        ///@todo in C++17 can use std::destroy()
        auto iter = begin();
        auto last = end();
        for (; iter != last; ++iter) {
          iter->~T();
        }
      }
      size_t bytes = Buffer<T>::size(volume());
      libbirch::deallocate(buffer, bytes, buffer->tid);
    }
    buffer = nullptr;
    offset = 0;
  }

  /**
   * Initialize allocated memory.
   *
   * @param args Constructor arguments.
   */
  template<class ... Args>
  void initialize(Args ... args) {
    auto iter = begin();
    auto last = end();
    for (; iter != last; ++iter) {
      new (&*iter) T(args...);
    }
  }

  /**
   * Assign from another array.
   */
  template<class U>
  void copy(const U& o) {
    auto n = std::min(size(), o.size());
    auto begin1 = o.begin();
    auto end1 = begin1 + n;
    auto begin2 = begin();
    auto end2 = begin2 + n;
    if (inside(begin1, end1, begin2)) {
      std::copy_backward(begin1, end1, end2);
    } else {
      std::copy(begin1, end1, begin2);
    }
  }

  /**
   * Copy from another array.
   */
  template<class U>
  void uninitialized_copy(const U& o) {
    assert(!isShared());
    auto n = std::min(size(), o.size());
    auto begin1 = o.begin();
    auto end1 = begin1 + n;
    auto begin2 = begin();
    for (; begin1 != end1; ++begin1, ++begin2) {
      new (&*begin2) T(*begin1);
    }
  }

  /**
   * Shape.
   */
  F shape;

  /**
   * Buffer.
   */
  Buffer<T>* buffer;

  /**
   * Offset into the buffer. This should be zero when isView is false.
   */
  int64_t offset;

  /**
   * Is this a view of another array? A view has stricter assignment
   * semantics, as it cannot be resized or moved.
   */
  bool isView;

  /**
   * Lock used for copy-on-write. Read use is obtained when the current
   * buffer must be preserved for either read or write operations. Write use
   * is obtained to substitute the current buffer with another.
   */
  ReadersWriterLock bufferLock;
};

template<class T, class F>
struct is_value<Array<T,F>> {
  static const bool value = is_value<T>::value;
};

template<class T, class F, unsigned N>
struct is_acyclic<Array<T,F>,N> {
  static const bool value = is_acyclic<T,N>::value;
};

/**
 * Default array for `D` dimensions.
 */
template<class T, int D>
using DefaultArray = Array<T,typename DefaultShape<D>::type>;

}
