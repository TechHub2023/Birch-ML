/**
 * @file
 */
#include "src/expression/Assign.hpp"

#include "src/visitor/all.hpp"

birch::Assign::Assign(Expression* left, Name* op, Expression* right,
    Location* loc) :
    Expression(loc),
    Named(op),
    Couple<Expression>(left, right) {
  //
}

birch::Expression* birch::Assign::accept(Modifier* visitor) {
  return visitor->modify(this);
}

void birch::Assign::accept(Visitor* visitor) const {
  visitor->visit(this);
}
