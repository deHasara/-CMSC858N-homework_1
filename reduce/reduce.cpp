#include "reduce.h"

#include <cstdlib>
#include <iostream>
#include <string.h>

#include "get_time.h"

#include <thread>

using Type = long long;

int main(int argc, char* argv[]) {
  auto num_threads = std::thread::hardware_concurrency();
  //std::cout<<"num threads: "<<num_threads <<std::endl;

  size_t n = 1e9;
  int num_rounds = 3;
  if (argc >= 2) {
    n = atoll(argv[1]);
  }
  if (argc >= 3) {
    num_rounds = atoi(argv[2]);
  }
  bool test = 0;
  if (argc >= 4){
    if (!strcmp(argv[3],"-t")){
      test = 1;
    }
    else{
      std::cerr << "Unknown flag: " << argv[3] << std::endl;
      exit(-1);
    }
  }
  if (!test){
    std::cout << "n = " << n << " num_rounds = " << num_rounds << std::endl;
  }
  Type* A = (Type*)malloc(n * sizeof(Type));
  parallel_for(0, n, [&](size_t i) { A[i] = i; });

  double total_time = 0;
  for (int i = 0; i <= num_rounds; i++) {
    parlay::timer t;
    //long long ans = reduce(A, n);
    long long ans_granularity = reduce_granularity(A, n);
    //long long ans = serial_reduce(A, n);
    t.stop();

    if (i == 0) {
      if (test){
        //std::cout << ans << std::endl;
        std::cout << ans_granularity << std::endl;
      }
      else{
        //std::cout << "Total sum: " << ans << std::endl;
        std::cout << "Total sum with granularity: "<< ans_granularity << std::endl;
        std::cout << "Warmup round running time: " << t.total_time() << std::endl;
      }
    } else {
      if (!test){
        std::cout << "Round " << i << " running time: " << t.total_time()
                  << std::endl;
      }
      total_time += t.total_time();
    }
  }
  if (!test){
    std::cout << "Average running time: " << total_time / num_rounds << std::endl;
  }

  free(A);

  return 0;
}
