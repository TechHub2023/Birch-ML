/**
 * @file
 */
#include "src/expression/Get.hpp"

#include "src/visitor/all.hpp"

birch::Get::Get(Expression* single, Location* loc) :
    Expression(loc),
    Single<Expression>(single) {
  //
}

bool birch::Get::isAssignable() const {
  return single->isAssignable();
}

birch::Expression* birch::Get::accept(Modifier* visitor) {
  return visitor->modify(this);
}

void birch::Get::accept(Visitor* visitor) const {
  return visitor->visit(this);
}
