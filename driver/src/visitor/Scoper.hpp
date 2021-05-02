/**
 * @file
 */
#pragma once

#include "src/visitor/ScopedModifier.hpp"

namespace birch {
/**
 * Populate global and member scopes.
 *
 * @ingroup visitor
 */
class Scoper: public ScopedModifier {
public:
  /**
   * Constructor.
   *
   * @param currentPackage If the visitor will not begin by visiting the
   * package, provide it for scoping purposes.
   * @param currentClass If the visitor will begin by visiting the members of
   * a class, but not the class itself, provide it for scoping purposes.
   */
  Scoper(Package* currentPackage = nullptr, Class* currentClass = nullptr);

  using ScopedModifier::modify;

  virtual Statement* modify(MemberVariable* o);
  virtual Statement* modify(GlobalVariable* o);
  virtual Statement* modify(MemberFunction* o);
  virtual Statement* modify(Function* o);
  virtual Statement* modify(BinaryOperator* o);
  virtual Statement* modify(UnaryOperator* o);
  virtual Statement* modify(Program* o);
  virtual Statement* modify(Basic* o);
  virtual Statement* modify(Class* o);
  virtual Expression* modify(Generic* o);
};
}
