/**
 * @file
 */
#include "libbirch/Lock.hpp"

void libbirch::Lock::share() {
  /* spin until exclusive lock is released and shared count updated */
  joint_lock_type expected = lock.joint.load(), desired;
  do {
    expected.keepCount = 0u;
    desired = {expected.shareCount + 1u, 0u};
  } while (!lock.joint.compare_exchange_weak(expected, desired, std::memory_order_seq_cst));
}

void libbirch::Lock::keep() {
  /* spin until exclusive lock obtained */
  unsigned expected;
  do {
    expected = 0u;
  } while (!lock.split.keepCount.compare_exchange_weak(expected, 1u, std::memory_order_seq_cst));

  /* spin until all threads with shared locks release */
  while (lock.split.shareCount.load(std::memory_order_seq_cst) > 0u);
}
