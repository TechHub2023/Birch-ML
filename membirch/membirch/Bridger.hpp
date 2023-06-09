/**
 * @file
 */
#pragma once

#include "membirch/external.hpp"
#include "membirch/internal.hpp"
#include "membirch/type.hpp"

namespace membirch {
/**
 * @internal
 * 
 * Visitor implementing the second pass of bridge finding.
 */
class Bridger {
public:
  static constexpr int MAX = std::numeric_limits<int>::max();

  std::tuple<int,int,int,int> visit(const int j, const int k) {
    return std::make_tuple(MAX, 0, 0, 0);
  }

  template<class T, std::enable_if_t<
      is_visitable<T,Bridger>::value,int> = 0>
  std::tuple<int,int,int,int> visit(const int j, const int k, T& o) {
    return o.accept_(*this, j, k);
  }

  template<class T, std::enable_if_t<
      !is_visitable<T,Bridger>::value &&
      is_iterable<T>::value,int> = 0>
  std::tuple<int,int,int,int> visit(const int j, const int k, T& o) {
    int l = MAX, h = 0, m = 0, n = 0, l1, h1, m1, n1;
    if (!std::is_trivial<typename T::value_type>::value) {
      auto iter = o.begin();
      auto last = o.end();
      for (; iter != last; ++iter) {
        std::tie(l1, h1, m1, n1) = visit(j + m, k + n, *iter);
        l = std::min(l, l1);
        h = std::max(h, h1);
        m += m1;
        n += n1;
      }
    }
    return std::make_tuple(l, h, m, n);
  }

  template<class T, std::enable_if_t<
      !is_visitable<T,Bridger>::value &&
      !is_iterable<T>::value,int> = 0>
  std::tuple<int,int,int,int> visit(const int j, const int k, T& o) {
    return std::make_tuple(MAX, 0, 0, 0);
  }

  template<class Arg, class... Args>
  std::tuple<int,int,int,int> visit(const int j, const int k, Arg& arg,
      Args&... args) {
    int l, h, m, n, l1, h1, m1, n1;
    std::tie(l, h, m, n) = visit(j, k, arg);
    std::tie(l1, h1, m1, n1) = visit(j + m, k + n, args...);
    l = std::min(l, l1);
    h = std::max(h, h1);
    m += m1;
    n += n1;
    return std::make_tuple(l, h, m, n);
  }

  template<class... Args>
  std::tuple<int,int,int,int> visit(const int j, const int k,
      std::tuple<Args...>& o) {
    return std::apply([&](Args&... args) { return visit(j, k, args...); }, o);
  }

  template<class T>
  std::tuple<int,int,int,int> visit(const int j, const int k,
      std::optional<T>& o) {
    if (o.has_value()) {
      return visit(j, k, o.value());
    } else {
      return std::make_tuple(MAX, 0, 0, 0);
    }
  }

  template<class T>
  std::tuple<int,int,int,int> visit(const int j, const int k, Shared<T>& o);

  std::tuple<int,int,int,int> visitObject(const int j, const int k, Any* o);
};
}

#include "membirch/Shared.hpp"

template<class T>
std::tuple<int,int,int,int> membirch::Bridger::visit(const int j, const int k,
    Shared<T>& o) {
  auto [ptr, bridge] = o.unpack();
  if (!bridge) {
    int l, h, m, n;
    std::tie(l, h, m, n) = visitObject(j, k, ptr);
    if (l == j && h < j + m) {
      /* is a bridge */
      o.setBridge();
      n = 0;  // base case for post-order rank in biconnected component
    }
    return std::make_tuple(l, h, m, n);
  } else {
    return std::make_tuple(MAX, 0, 0, 0);
  }
}
