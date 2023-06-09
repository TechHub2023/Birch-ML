/**
 * @file
 */
#include "membirch/memory.hpp"

#include "membirch/thread.hpp"
#include "membirch/Atomic.hpp"
#include "membirch/Any.hpp"
#include "membirch/Marker.hpp"
#include "membirch/Scanner.hpp"
#include "membirch/Collector.hpp"

#include <vector>

/**
 * Possible roots list for each thread.
 */
static thread_local std::vector<membirch::Any*> possible_roots;

/**
 * Unreachable list for each thread.
 */
static thread_local std::vector<membirch::Any*> unreachables;

/**
 * Copy flag for each thread.
 */
static thread_local bool copy_flag = false;

void membirch::register_possible_root(Any* o) {
  possible_roots.push_back(o);
}

void membirch::deregister_possible_root(Any* o) {
  assert(o->numShared_() == 0);
  if (!possible_roots.empty() && possible_roots.back() == o) {
    /* easy to deallocate an object if it was the most-recently added */
    possible_roots.pop_back();
    o->deallocate_();
  }
}

bool membirch::contains_possible_root(Any* o) {
  return std::find(possible_roots.begin(), possible_roots.end(), o) !=
      possible_roots.end();
}

void membirch::register_unreachable(Any* o) {
  unreachables.push_back(o);
}

void membirch::collect() {
  /* concatenates the possible roots list of each thread into a single list,
   * then distributes the passes over this list between all threads; this
   * improves load balancing over each thread operating only on its original
   * list */

  auto nthreads = get_max_threads();

  /* concatenated list of possibles root objects, and unreachable objects */
  std::vector<membirch::Any*> all_possible_roots, all_unreachables;

  /* start and end indices for each thread in concatenated lists */
  std::vector<int> starts(nthreads), sizes(nthreads);

  #pragma omp parallel
  {
    auto tid = get_thread_num();

    /* objects can be added to the possible roots list during normal
     * execution, but not removed, although they may be flagged as no longer
     * being a possible root; remove such objects first */
    int size = 0;
    for (int i = 0; i < (int)possible_roots.size(); ++i) {
      auto o = possible_roots[i];
      if (o->numShared_() == 0) {
        o->deallocate_();  // deallocation was deferred until now
      } else if (o->isPossibleRoot_()) {
        possible_roots[size++] = o;
      } else {
        o->unbuffer_();  // not a root, mark as no longer in the buffer
      }
    }
    possible_roots.resize(size);
    sizes[tid] = size;
    #pragma omp barrier

    /* a single thread now sets up the concatenated list of possible roots */
    #pragma omp single
    {
      #ifdef __cpp_lib_parallel_algorithm
      std::exclusive_scan(sizes.begin(), sizes.end(), starts.begin(), 0);
      #else
      starts[0] = 0;
      for (int i = 1; i < nthreads; ++i) {
        starts[i] = starts[i - 1] + sizes[i - 1];
      }
      #endif
      all_possible_roots.resize(starts.back() + sizes.back());
    }
    #pragma omp barrier

    /* all threads copy into the concatenated list of possible roots */
    std::copy(possible_roots.begin(), possible_roots.end(),
        all_possible_roots.begin() + starts[tid]);
    possible_roots.clear();
    #pragma omp barrier

    /* mark pass */
    #pragma omp for schedule(dynamic)
    for (int i = 0; i < (int)all_possible_roots.size(); ++i) {
      auto o = all_possible_roots[i];
      Marker visitor;
      visitor.visitObject(o);
    }
    #pragma omp barrier

    /* scan/reach pass */
    #pragma omp for schedule(dynamic)
    for (int i = 0; i < (int)all_possible_roots.size(); ++i) {
      auto o = all_possible_roots[i];
      Scanner visitor;
      visitor.visitObject(o);
    }
    #pragma omp barrier

    /* collect pass */
    #pragma omp for schedule(dynamic)
    for (int i = 0; i < (int)all_possible_roots.size(); ++i) {
      auto o = all_possible_roots[i];
      Collector visitor;
      visitor.visitObject(o);
    }
    sizes[tid] = unreachables.size();
    #pragma omp barrier

    /* a single thread now sets up the concatenated list of unreachables */
    #pragma omp single
    {
      #ifdef __cpp_lib_parallel_algorithm
      std::exclusive_scan(sizes.begin(), sizes.end(), starts.begin(), 0);
      #else
      starts[0] = 0;
      for (int i = 1; i < nthreads; ++i) {
        starts[i] = starts[i - 1] + sizes[i - 1];
      }
      #endif
      all_unreachables.resize(starts.back() + sizes.back());
    }
    #pragma omp barrier

    /* all threads copy into the concatenated list of unreachables */
    std::copy(unreachables.begin(), unreachables.end(),
        all_unreachables.begin() + starts[tid]);
    unreachables.clear();
    #pragma omp barrier

    /* finally, destroy objects determined unreachable */
    #pragma omp for schedule(dynamic)
    for (int i = 0; i < (int)all_unreachables.size(); ++i) {
      auto o = all_unreachables[i];
      o->destroy_();
      o->deallocate_();
    }
  }

  assert(possible_roots.empty());
  assert(unreachables.empty());
}

bool membirch::in_copy() {
  return copy_flag;
}

void membirch::set_copy() {
  copy_flag = true;
}

void membirch::unset_copy() {
  copy_flag = false;
}

void membirch::biconnected_collect(Any* o) {
  BiconnectedCollector().visitObject(o);
}
