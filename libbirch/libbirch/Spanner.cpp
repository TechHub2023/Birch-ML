/**
 * @file
 */
#include "libbirch/Spanner.hpp"

std::tuple<int,int,int> libbirch::Spanner::visit(const int i, const int j,
    Any* o) {
  if (!(o->f.exchangeOr(CLAIMED) & CLAIMED)) {
    /* just claimed by this thread */
    ++o->a;
    o->p = get_thread_num();
    o->n = j;
    o->l = j;
    o->h = j;
    int l, h, k;
    std::tie(l, h, k) = o->accept_(*this, j, j + 1);
    o->l = std::min(o->l, l);
    o->h = std::max(o->h, h);
    return std::make_tuple(j, j, k + 1);
  } else if (o->p == get_thread_num()) {
    /* previously claimed by this thread */
    ++o->a;
    o->l = std::min(o->l, i);
    o->h = std::max(o->h, i);
    int n = o->n;  // avoids compile error when o->n encoded as bitfield
    return std::make_tuple(n, n, 0);
  } else {
    /* claimed by a different thread */
    return std::make_tuple(i, i, 0);
  }
}