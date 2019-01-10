/**
 * @file
 */
#include "libbirch/Any.hpp"

#include "libbirch/memory.hpp"

bi::Any::Any() :
    context(currentContext),
    forward(nullptr),
	  frozen(false) {
  //
}

bi::Any::Any(const Any& o) :
    Counted(o),
    context(currentContext),
    forward(nullptr),
	  frozen(false) {
  //
}

bi::Any::~Any() {
  Any* forward = this->forward;  // load from atomic just once
  if (forward) {
    forward->decShared();
  }
}

bool bi::Any::isFrozen() const {
  return frozen;
}

void bi::Any::freeze() {
  frozen = true;
}

bi::Any* bi::Any::getForward() {
  if (frozen) {
    Any* forward = this->forward;  // load from atomic just once
    if (!forward) {
      SwapClone swapClone(true);
      SwapContext swapContext(context.get());
      Any* cloned = this->clone();
      cloned->incShared();
      if (this->forward.compare_exchange_strong(forward, cloned)) {
        return cloned;
      } else {
        /* beaten by another thread */
        cloned->decShared();
      }
    }
    return forward;
  } else {
    return this;
  }
}

bi::Any* bi::Any::pullForward() {
  if (frozen) {
    Any* forward = this->forward;  // load from atomic just once
    if (forward) {
      return forward;
    }
  }
  return this;
}

bi::Memo* bi::Any::getContext() {
  return context.get();
}
