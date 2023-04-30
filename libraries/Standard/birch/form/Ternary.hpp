/**
 * @file
 */
#pragma once

#include "birch/form/Form.hpp"

namespace birch {
/**
 * Delayed form with three arguments.
 */
template<class Left, class Middle, class Right>
struct Ternary : public Form {
  template<class T, class U, class V>
  Ternary(T&& l, U&& m, V&& r) :
      Form(),
      l(l),
      m(m),
      r(r) {
    //
  }

  void reset() {
    birch::reset(l);
    birch::reset(m);
    birch::reset(r);
  }

  void relink(const RelinkVisitor& visitor) {
    birch::relink(l, visitor);
    birch::relink(m, visitor);
    birch::relink(r, visitor);
  }

  void constant() {
    birch::constant(l);
    birch::constant(m);
    birch::constant(r);
  }

  bool isConstant() const {
    return birch::is_constant(l) && birch::is_constant(m) &&
        birch::is_constant(r);
  }

  void args(const ArgsVisitor& visitor) {
    birch::args(l, visitor);
    birch::args(m, visitor);
    birch::args(r, visitor);
  }

  void deepGrad(const GradVisitor& visitor) {
    birch::deep_grad(l, visitor);
    birch::deep_grad(m, visitor);
    birch::deep_grad(r, visitor);
  }

  /**
   * Left argument.
   */
  Left l;

  /**
   * Middle argument.
   */
  Middle m;
  
  /**
   * Right argument.
   */
  Right r;

  MEMBIRCH_STRUCT(Ternary, Form)
  MEMBIRCH_STRUCT_MEMBERS(l, m, r)
};

}

#define BIRCH_TERNARY_FORM(f, ...) \
  using Value = decltype(f(birch::eval(std::declval<Left>()), \
      birch::eval(std::declval<Middle>()), \
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
    auto m = birch::eval(this->m); \
    auto r = birch::eval(this->r); \
    return f(l, m, r, ##__VA_ARGS__); \
  } \
  \
  auto peek() { \
    if (!x) { \
      auto l = birch::peek(this->l); \
      auto m = birch::peek(this->m); \
      auto r = birch::peek(this->r); \
      this->x = f(l, m, r, ##__VA_ARGS__); \
    } \
    return *x; \
  } \
  \
  auto move(const MoveVisitor& visitor) { \
    auto l = birch::move(this->l, visitor); \
    auto m = birch::move(this->m, visitor); \
    auto r = birch::move(this->r, visitor); \
    return f(l, m, r, ##__VA_ARGS__); \
  }

#define BIRCH_TERNARY_GRAD(f_grad, ...) \
  template<class G> \
  void shallowGrad(const G& g, const GradVisitor& visitor) { \
    auto x = birch::peek(*this); \
    auto l = birch::peek(this->l); \
    auto m = birch::peek(this->m); \
    auto r = birch::peek(this->r); \
    birch::shallow_grad(this->l, f_grad ## 1(g, x, l, m, r, ##__VA_ARGS__), visitor); \
    birch::shallow_grad(this->m, f_grad ## 2(g, x, l, m, r, ##__VA_ARGS__), visitor); \
    birch::shallow_grad(this->r, f_grad ## 3(g, x, l, m, r, ##__VA_ARGS__), visitor); \
    clear(); \
  }