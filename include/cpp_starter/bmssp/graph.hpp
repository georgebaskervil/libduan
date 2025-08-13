#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

namespace bmssp {

struct Edge { int to; uint64_t w; };

class Graph {
public:
  Graph() = default;
  explicit Graph(std::size_t n) : adj_(n) {}
  std::size_t size() const noexcept { return adj_.size(); }
  void add_edge(int u, int v, uint64_t w) { adj_[u].push_back({v,w}); }
  const std::vector<Edge>& neighbors(int u) const { return adj_[u]; }
private:
  std::vector<std::vector<Edge>> adj_;
};

} // namespace bmssp
