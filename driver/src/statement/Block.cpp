/**
 * @file
 */
#include "src/statement/Block.hpp"

#include "src/visitor/all.hpp"

birch::Block::Block(Statement* braces,Location* loc) :
    Statement(loc),
    Braced(braces) {
  //
}

birch::Statement* birch::Block::accept(Modifier* visitor) {
  return visitor->modify(this);
}

void birch::Block::accept(Visitor* visitor) const {
  visitor->visit(this);
}
