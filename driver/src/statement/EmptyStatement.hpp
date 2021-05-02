/**
 * @file
 */
#pragma once

#include "src/statement/Statement.hpp"

namespace birch {
/**
 * Empty statement.
 *
 * @ingroup statement
 */
class EmptyStatement: public Statement {
public:
  /**
   * Constructor.
   *
   * @param loc Location.
   */
  EmptyStatement(Location* loc = nullptr);

  virtual Statement* accept(Modifier* visitor);
  virtual void accept(Visitor* visitor) const;

  virtual bool isEmpty() const;
};
}
