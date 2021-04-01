/**
 * @file
 */
#pragma once

#include "src/common/Located.hpp"

namespace birch {
class Cloner;
class Modifier;
class Visitor;
class StatementIterator;

/**
 * Statement.
 *
 * @ingroup statement
 */
class Statement: public Located {
public:
  /**
   * Constructor.
   *
   * @param loc Location.
   */
  Statement(Location* loc = nullptr);

  /**
   * Destructor.
   */
  virtual ~Statement() = 0;

  /**
   * Accept cloning visitor.
   *
   * @param visitor The visitor.
   *
   * @return Cloned (and potentially modified) statement.
   */
  virtual Statement* accept(Cloner* visitor) const = 0;

  /**
   * Accept modifying visitor.
   *
   * @param visitor The visitor.
   *
   * @return Modified statement.
   */
  virtual Statement* accept(Modifier* visitor) = 0;

  /**
   * Accept read-only visitor.
   *
   * @param visitor The visitor.
   */
  virtual void accept(Visitor* visitor) const = 0;

  /**
   * Strip braces, if any.
   */
  virtual Statement* strip();

  /**
   * Number of elements when iterating.
   */
  virtual int count() const;

  /**
   * Is this an empty statement?
   */
  virtual bool isEmpty() const;

  /**
   * Iterator to first element if this is a list, to one-past-the-last if
   * this is empty, otherwise to this.
   */
  StatementIterator begin() const;

  /**
   * Iterator to one-past-the-last.
   */
  StatementIterator end() const;
};
}
