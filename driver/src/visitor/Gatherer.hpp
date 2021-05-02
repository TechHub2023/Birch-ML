/**
 * @file
 */
#pragma once

#include "src/visitor/Visitor.hpp"

namespace birch {
/**
 * Gathers object of a given type from an abstract syntax tree.
 *
 * @ingroup visitor
 */
template<class T>
class Gatherer: public Visitor {
public:
  /**
   * Constructor.
   *
   * @param predicate Optional predicate function to filter objects of type T.
   * @param headers For packages, recurse into header files as well (or just
   * source files)?
   */
  Gatherer(std::function<bool(const T*)> predicate =
      [](const T* o) -> bool {return true;}, const bool headers = true);

  /**
   * Begin iterator over gathered objects.
   */
  auto begin() {
    return gathered.begin();
  }

  /**
   * End iterator over gathered objects.
   */
  auto end() {
    return gathered.end();
  }

  /**
   * Number of items gathered.
   */
  auto size() {
    return gathered.size();
  }

  using Visitor::visit;
  virtual void visit(const Package* o);
  virtual void visit(const T* o);

protected:
  /**
   * Predicate.
   */
  std::function<bool(const T*)> predicate;

  /**
   * Recurse into headers?
   */
  bool headers;

  /**
   * Gathered objects.
   */
  std::vector<T*> gathered;
};
}

template<class T>
birch::Gatherer<T>::Gatherer(std::function<bool(const T*)> predicate, const bool headers) :
    predicate(predicate), headers(headers) {
  //
}

template<class T>
void birch::Gatherer<T>::visit(const Package* o) {
  if (headers) {
    for (auto file : o->headers) {
      file->accept(this);
    }
  }
  for (auto file : o->sources) {
    file->accept(this);
  }
}

template<class T>
void birch::Gatherer<T>::visit(const T* o) {
  if (predicate(o)) {
    gathered.push_back(const_cast<T*>(o));
  }
  Visitor::visit(o);
}
