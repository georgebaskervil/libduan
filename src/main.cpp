// License: Placeholder - add your license details here.
#include <iostream>
#include <vector>

#include "cpp_starter/jemalloc_allocator.hpp"
#include "cpp_starter/lib.hpp"

int main() {
  using cpp_starter::sum;
  int a = 2, b = 3;
  std::cout << "Hello from duansalgorithm!\n";
  std::cout << "sum(" << a << ", " << b << ") = " << sum(a, b) << "\n";
  // Demonstrate using jemalloc-backed allocator (falls back to default if not available)
  std::vector<int, cpp_starter::JemallocAllocator<int>> vec;
  for (int i = 0; i < 5; ++i) vec.push_back(i);
  std::cout << "jemalloc vector size=" << vec.size() << "\n";
  return 0;
}
