/**
 * @file
 */
#include "src/type/Type.hpp"

#include "src/type/TypeIterator.hpp"
#include "src/type/TypeConstIterator.hpp"
#include "src/exception/all.hpp"
#include "src/visitor/all.hpp"

birch::Type::Type(Location* loc) :
    Located(loc) {
  //
}

bool birch::Type::isEmpty() const {
  return false;
}

bool birch::Type::isArray() const {
  return false;
}

bool birch::Type::isTuple() const {
  return false;
}

bool birch::Type::isMember() const {
  return false;
}

bool birch::Type::isOptional() const {
  return false;
}

bool birch::Type::isFuture() const {
  return false;
}

bool birch::Type::isDeduced() const {
  return false;
}

int birch::Type::width() const {
  int result = 0;
  for (auto iter = begin(); iter != end(); ++iter) {
    ++result;
  }
  return result;
}

int birch::Type::depth() const {
  return 0;
}

birch::Type* birch::Type::unwrap() {
  return this;
}

const birch::Type* birch::Type::unwrap() const {
  return this;
}

birch::TypeIterator birch::Type::begin() {
  if (isEmpty()) {
    return end();
  } else {
    return TypeIterator(this);
  }
}

birch::TypeConstIterator birch::Type::begin() const {
  if (isEmpty()) {
    return end();
  } else {
    return TypeConstIterator(this);
  }
}

birch::TypeIterator birch::Type::end() {
  return TypeIterator(nullptr);
}

birch::TypeConstIterator birch::Type::end() const {
  return TypeConstIterator(nullptr);
}
