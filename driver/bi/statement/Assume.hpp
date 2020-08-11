/**
 * @file
 */
#pragma once

#include "bi/statement/Statement.hpp"
#include "bi/common/Named.hpp"
#include "bi/common/Couple.hpp"
#include "bi/expression/Expression.hpp"

namespace bi {
/**
 * Statement using `~` or `<-?` operator.
 *
 * @ingroup statement
 */
class Assume: public Statement, public Named, public Couple<Expression> {
public:
  /**
   * Constructor.
   *
   * @param left Left operand.
   * @param op Operator.
   * @param right Right operand.
   * @param loc Location.
   */
  Assume(Expression* left, Name* op, Expression* right, Location* loc = nullptr);

  /**
   * Destructor.
   */
  virtual ~Assume();

  virtual Statement* accept(Cloner* visitor) const;
  virtual Statement* accept(Modifier* visitor);
  virtual void accept(Visitor* visitor) const;
};
}