#include "parallel.h"

#include <future>
#include <numeric>
#include <algorithm>



using namespace parlay;

// A serial implementation for checking correctness.
//
// Work = O(n)
// Depth = O(n)
template <class T, class F>
T scan_inplace_serial(T *A, size_t n, const F& f, T id) {
  T cur = id;
  std::cout << "cur: " << cur << std::endl;

  for (size_t i=0; i<n; ++i) {
    T next = f(cur, A[i]);
    A[i] = cur;
    cur = next;
  }
  return cur;
}

// Parallel in-place prefix sums. Your implementation can allocate and
// use an extra n*sizeof(T) bytes of memory.
//
// The work/depth bounds of your implementation should be:
// Work = O(n)
// Depth = O(\log(n))
/*
template <class T, class F>
T scan_inplace(T *A, size_t n, const F& f, T id) {
  return id;  // TODO
}
 */
auto MinSize = 100;

template <typename Input, typename Output, typename F>
Output scan_inplace(Input begin, Input end, Output to, F function){
    auto n = std::distance(begin, end);
    if (0<n){
        to[0] = begin[0];
        if(1<n){
            std::vector<std::decay_t<decltype(*begin)>> t(n);
            scan_up(begin + 1, end, t.begin(), function);
            scan_down(begin[0], begin + 1, end, t.begin(), to + 1, function);
        }
    }
    return to + n;
}

template <typename Input, typename Output, typename F, typename Value>
Output inclusive_scan(Input it, Input end, Output to, F function, Value value) {
    for (; it != end; ++it, ++to)
        *to = value = function(value, *it);
    return to;
}

template <typename Input, typename Tmp, typename F>
auto scan_up(Input b, Input end, Tmp tmp, F function) {
    auto n = std::distance(b, end);
    if (n < MinSize) { return std::accumulate(b + 1, end, *b, function); }
    else {
        auto k = n / 2;
        auto fut = std::async([&]{ tmp[k] = scan_up(b, b + k, tmp, function); });
        auto right = scan_up(b + k, end, tmp + k, function);
        fut.wait();
        return function(tmp[k], right);
    }
}

template <typename V, typename Input, typename T, typename To, typename F>
void scan_down(V v, Input b, Input end, T tmp, To to, F function) {
    auto n = std::distance(b, end);
    if (n < MinSize) { inclusive_scan(b, end, to, function, v); }
    else {
        auto k = n / 2;
        auto fut = std::async([&]{ scan_down(v, b, b + k, tmp, to, function); });
        scan_down(function(v, tmp[k]), b + k, end, tmp + k, to + k, function);
        fut.wait();
    }
}





