/**
 * @file
 */
#pragma once

namespace birch {
class Expression;
class Type;

/**
 * Object with generic type parameters.
 *
 * @ingroup common
 */
class TypeParameterised {
public:
  /**
   * Constructor.
   *
   * @param typeParams Generic type parameters.
   */
  TypeParameterised(Expression* typeParams);

  /**
   * Does this class have generic type parameters?
   */
  bool isGeneric() const;

  /**
   * Create type arguments corresponding to the type parameters.
   */
  Type* createArguments() const;

  /**
   * Generic type parameters.
   */
  Expression* typeParams;
};
}
