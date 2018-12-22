/**
 * @file
 *
 * Standard headers that should be included for all C++ files generated by
 * the compiler.
 */
#pragma once

#include "libbirch/config.hpp"
#include "libbirch/assert.hpp"
#include "libbirch/basic.hpp"
#include "libbirch/class.hpp"
#include "libbirch/memory.hpp"
#include "libbirch/stacktrace.hpp"

#include "libbirch/Span.hpp"
#include "libbirch/Index.hpp"
#include "libbirch/Range.hpp"
#include "libbirch/Frame.hpp"
#include "libbirch/View.hpp"
#include "libbirch/Array.hpp"
#include "libbirch/Sequence.hpp"
#include "libbirch/Any.hpp"
#include "libbirch/Optional.hpp"
#include "libbirch/Nil.hpp"
#include "libbirch/SharedCOW.hpp"
#include "libbirch/WeakCOW.hpp"
#include "libbirch/Allocator.hpp"
#include "libbirch/FiberState.hpp"
#include "libbirch/Fiber.hpp"
#include "libbirch/Eigen.hpp"
#include "libbirch/EigenFunctions.hpp"
#include "libbirch/EigenOperators.hpp"

namespace bi {
/**
 * Default array for `D` dimensions.
 */
template<class T, int D>
using DefaultArray = Array<T,typename DefaultFrame<D>::type>;

/**
 * Default view for `D`-dimensional indexing of a single element.
 */
template<int D>
struct DefaultView {
  typedef NonemptyView<Index<>,typename DefaultView<D - 1>::type> type;
};
template<>
struct DefaultView<0> {
  typedef EmptyView type;
};

/**
 * Make a range.
 *
 * @ingroup libbirch
 *
 * @param start First index.
 * @param end Last index.
 */
inline Range<> make_range(const int64_t start, const int64_t end) {
  int64_t length = std::max(int64_t(0), end - start + 1);
  return Range<>(start, length);
}

/**
 * Make a frame, no arguments.
 *
 * @ingroup libbirch
 */
inline EmptyFrame make_frame() {
  return EmptyFrame();
}

/**
 * Make a frame, single argument.
 *
 * @ingroup libbirch
 */
inline NonemptyFrame<Span<>,EmptyFrame> make_frame(const int64_t arg) {
  auto tail = EmptyFrame();
  auto head = Span<>(arg, tail.volume());
  return NonemptyFrame<Span<>,EmptyFrame>(head, tail);
}

/**
 * Make a frame, multiple arguments.
 *
 * @ingroup libbirch
 */
template<class ... Args>
auto make_frame(const int64_t arg, Args ... args) {
  auto tail = make_frame(args...);
  auto head = Span<>(arg, tail.volume());
  return NonemptyFrame<decltype(head),decltype(tail)>(head, tail);
}

/**
 * Make a view, no arguments.
 *
 * @ingroup libbirch
 */
inline EmptyView make_view() {
  return EmptyView();
}

/**
 * Make a view, single argument.
 *
 * @ingroup libbirch
 */
template<int64_t offset_value, int64_t length_value>
auto make_view(const Range<offset_value,length_value>& arg) {
  auto head = arg;
  auto tail = make_view();
  return NonemptyView<decltype(head),decltype(tail)>(head, tail);
}

/**
 * Make a view, single argument.
 *
 * @ingroup libbirch
 */
inline NonemptyView<Index<>,EmptyView> make_view(const int64_t arg) {
  auto head = Index<>(arg);
  auto tail = EmptyView();
  return NonemptyView<Index<>,EmptyView>(head, tail);
}

/**
 * Make a view, multiple arguments.
 *
 * @ingroup libbirch
 */
template<int64_t offset_value, int64_t length_value, class ... Args>
auto make_view(const Range<offset_value,length_value>& arg, Args ... args) {
  auto head = arg;
  auto tail = make_view(args...);
  return NonemptyView<decltype(head),decltype(tail)>(head, tail);
}

/**
 * Make a view, multiple arguments.
 *
 * @ingroup libbirch
 */
template<class ... Args>
auto make_view(const int64_t arg, Args ... args) {
  auto head = Index<mutable_value>(arg);
  auto tail = make_view(args...);
  return NonemptyView<decltype(head),decltype(tail)>(head, tail);
}

/**
 * Make an array.
 *
 * @ingroup libbirch
 *
 * @tparam Type Value type.
 * @tparam Frame Frame type.
 * @tparam Args Constructor parameter types.
 *
 * @param frame Frame.
 * @param args Constructor arguments.
 *
 * @return The array.
 */
template<class Type, class Frame, class ... Args>
auto make_array(const Frame& frame, Args ... args) {
  return Array<Type,Frame>(frame, args...);
}

/**
 * Make an array and assign a value to it.
 *
 * @ingroup libbirch
 *
 * @tparam Type Value type.
 * @tparam Frame Frame type.
 * @tparam Value Initial value type.
 *
 * @param frame Frame.
 * @param value Initial value.
 *
 * @return The array.
 */
template<class Type, class Frame, class Value>
auto make_array_and_assign(const Frame& frame, const Value& value) {
  Frame zero;
  auto result = Array<Type,Frame>(zero);
  result.enlarge(frame, value);
  return result;
}

/**
 * Make a pointer to a new object.
 *
 * @ingroup libbirch
 *
 * @tparam PointerType Pointer type.
 * @tparam Args Constructor parameter types.
 *
 * @param args Constructor arguments.
 *
 * @return The pointer to the new object.
 */
template<class PointerType, class ... Args>
PointerType make_pointer(Args ... args) {
  return PointerType::value_type::create(args...);
}

/**
 * Make a fiber.
 *
 * @tparam StateType The state type of the fiber.
 * @tparam Args Fiber state constructor parameter types.
 *
 * @param args Fiber state constructor arguments.
 */
template<class StateType, class ... Args>
auto make_fiber(Args ... args) {
  using yield_type = typename StateType::yield_type;
  return Fiber<yield_type>(StateType::create(args...));
}

/**
 * Cast an object.
 */
template<class To, class From>
auto dynamic_pointer_cast(const SharedCOW<From>& from) {
  return Optional<SharedCOW<To>>(std::move(from.template dynamic_pointer_cast<To>()));
}

/**
 * Cast an object optional.
 */
template<class To, class From>
auto dynamic_pointer_cast(const Optional<SharedCOW<From>>& from) {
  if (from.query()) {
    return dynamic_pointer_cast<To>(from.get());
  } else {
    return Optional<SharedCOW<To>>();
  }
}

/**
 * Cast anything else.
 *
 * @return An optional, with a value only if @p from is of type To.
 */
template<class To, class From>
auto check_cast(const From& from) {
  return std::is_same<To,From>::value ? Optional<To>(from) : Optional<To>();
}

/**
 * Cast an optional of anything else.
 *
 * @return An optional, with a value only if @p from has a value of type To.
 */
template<class To, class From>
auto check_cast(const Optional<From>& from) {
  if (from.query()) {
    return check_cast<To>(from.get());
  } else {
    return Optional<To>();
  }
}

/**
 * Report unknown program option and abort.
 */
void unknown_option(const std::string& name);

}
