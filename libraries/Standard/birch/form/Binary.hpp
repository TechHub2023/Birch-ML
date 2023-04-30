/**
 * @file
 */
#pragma once

#include "birch/form/Form.hpp"

namespace birch {
/**
 * Delayed form with two arguments.
 */
template<class Left, class Right>
struct Binary : public Form {
  template<class T, class U>
  Binary(T&& l, U&& r) :
      Form(),
      l(l),
      r(r) {
    //
  }

  void reset() {
    birch::reset(l);
    birch::reset(r);
  }

  void relink(const RelinkVisitor& visitor) {
    birch::relink(l, visitor);
    birch::relink(r, visitor);
  }

  void constant() {
    birch::constant(l);
    birch::constant(r);
  }

  bool isConstant() const {
    return birch::is_constant(l) && birch::is_constant(r);
  }

  void args(const ArgsVisitor& visitor) {
    birch::args(l, visitor);
    birch::args(r, visitor);
  }

  void deepGrad(const GradVisitor& visitor) {
    birch::deep_grad(l, visitor);
    birch::deep_grad(r, visitor);
  }

  /**
   * Left argument.
   */
  Left l;

  /**
   * Right argument.
   */
  Right r;

  MEMBIRCH_STRUCT(Binary, Form)
  MEMBIRCH_STRUCT_MEMBERS(l, r)
};

}

#define BIRCH_BINARY_FORM(f, ...) \
  using Value = decltype(f(birch::eval(std::declval<Left>()), \
        birch::eval(std::declval<Right>()), ##__VA_ARGS__)); \
  std::optional<Value> x; \
  \
  void clear() { \
    x.reset(); \
  } \
  \
  auto value() { \
    auto x = this->eval(); \
    this->constant(); \
    return x; \
  } \
  \
  auto eval() const { \
    auto l = birch::eval(this->l); \
    auto r = birch::eval(this->r); \
    return f(l, r, ##__VA_ARGS__); \
  } \
  \
  auto peek() { \
    if (!x) { \
      auto l = birch::peek(this->l); \
      auto r = birch::peek(this->r); \
      this->x = f(l, r, ##__VA_ARGS__); \
    } \
    return *x; \
  } \
  \
  auto move(const MoveVisitor& visitor) { \
    auto l = birch::move(this->l, visitor); \
    auto r = birch::move(this->r, visitor); \
    return f(l, r, ##__VA_ARGS__); \
  }

#define BIRCH_BINARY_GRAD(f_grad, ...) \
  template<class G> \
  void shallowGrad(const G& g, const GradVisitor& visitor) { \
    auto x = birch::peek(*this); \
    auto l = birch::peek(this->l); \
    auto r = birch::peek(this->r); \
    birch::shallow_grad(this->l, f_grad ## 1(g, x, l, r, ##__VA_ARGS__), visitor); \
    birch::shallow_grad(this->r, f_grad ## 2(g, x, l, r, ##__VA_ARGS__), visitor); \
    clear(); \
  }