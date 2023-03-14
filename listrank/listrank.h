#include "parallel.h"
#include "random.h"

// For timing parts of your code.
#include "get_time.h"

// For computing sqrt(n)
#include <math.h>
#include "mutex"

using namespace parlay;

// Some helpful utilities
namespace {

// returns the log base 2 rounded up (works on ints or longs or unsigned
// versions)
template <class T>
size_t log2_up(T i) {
  assert(i > 0);
  size_t a = 0;
  T b = i - 1;
  while (b > 0) {
    b = b >> 1;
    a++;
  }
  return a;
}

}  // namespace


struct ListNode {
  ListNode* next;
  size_t rank;
  ListNode(ListNode* next) : next(next), rank(std::numeric_limits<size_t>::max()) {}
};

// Serial List Ranking. The rank of a node is its distance from the
// tail of the list. The tail is the node with `next` field nullptr.
//
// The work/depth bounds are:
// Work = O(n)
// Depth = O(n)
void SerialListRanking(ListNode* head) {
  size_t ctr = 0;
  ListNode* save = head;
  while (head != nullptr) {
    head = head->next;
    ++ctr;
  }
  head = save;
  --ctr;  // last node is distance 0
  while (head != nullptr) {
    head->rank = ctr;
    head = head->next;
    --ctr;
  }
}

// Wyllie's List Ranking. Based on pointer-jumping.
//
// The work/depth bounds of your implementation should be:
// Work = O(n*\log(n))
// Depth = O(\log^2(n))
void WyllieListRanking(ListNode* L, size_t n) {

    //ListNode* save = L;
    //ListNode* head =L;
    //ListNode* save_first = &save[0];
    //ListNode* head_first = &head[0];

    /*for(int i=0; i<n; i++){
        if (head->next == NULL){
            head->rank = 0;
        }else{
            head->rank = 1;
        }
        head=head->next;
    }

    while (head->next!=NULL){
        head->rank = 1;
        head = head->next;
    }
    head->rank=0;

    head = head_first;

    while (head_first->next!=NULL){
        for(size_t i=0; i<n; i++){
            if (head->next!=NULL){
                head->rank = head->rank + head->next->rank;
                head->next = head->next->next;
            }
            head=head->next;
        }
    }
     */

    std::mutex mutex;

    parlay::parallel_for(0, n, [&](size_t i){
        //mutex is automatically released when lock goes out of scope
        //std::lock_guard<std::mutex> lock(mutex);
        if(L[i].next != nullptr) {
            L[i].rank = 1;
        }else{
            L[i].rank = 0;
        }
    });

    for(size_t i=0;i<log2_up(n);i++){
        parlay::parallel_for(0, n, [&](size_t j){
            //std::lock_guard<std::mutex> lock(mutex);
            if(L[j].next != nullptr){
                L[j].rank = L[j].rank + L[j].next->rank;
                L[j].next = L[j].next->next;
            }
        });
    }

}


// Sampling-Based List Ranking
//
// The work/depth bounds of your implementation should be:
// Work = O(n) whp
// Depth = O(\sqrt(n)* \log(n)) whp

struct ListNodeSampling {
    ListNodeSampling* next;
    size_t rank;
    size_t rand;
    bool head = false;
    size_t dist=1;
    size_t time;
    ListNodeSampling* previous;

};

void SamplingBasedListRanking(ListNodeSampling* L, size_t n) {
  // Perhaps use a serial base case for small enough inputs?

    size_t t =1;
    size_t j =0;
    ListNodeSampling* cell = L;
    cell->head = true;

    std::mutex mutex;

    //splice

    parlay::parallel_for(1, log2_up(n), [&](size_t i){
        cell[i].rand = rand()%2;
        std::lock_guard<std::mutex> lock(mutex);
        if (cell[i].rand==1 and (cell[i].next->head == false or cell[i].next->rand==0)){
            cell[i].previous->next=cell[i].next;
            cell[i].next->previous=cell[i].previous;
            cell[i].previous->dist=cell[i].previous->dist + cell[i].dist;
            cell[i].time = t;
            i = i+1;
            cell = &L[i];
            cell[i+1].head = true;
        }
        t = t+1;
    });


    //reconstruct
    j = log2_up(n);
    cell = &L[log2_up(n)];
    t=t;
    while (j>=0){
        if(cell->time==t){
            cell->rank = cell->next->rank + cell->dist;
            j=j-1;
            cell = &L[j];
        }
        t=t-1;
    }
}

