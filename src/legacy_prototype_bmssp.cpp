#include "cpp_starter/bmssp.hpp"
#include <queue>
#include <functional>
#include <unordered_map>

// Legacy prototype retained for reference; suppress noisy conversion warnings.
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wsign-conversion"
# pragma clang diagnostic ignored "-Wshorten-64-to-32"
# pragma clang diagnostic ignored "-Wconversion"
# pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif

namespace cpp_starter {
// (Legacy prototype implementation retained verbatim from previous lib.cpp)
namespace {
inline bool relax_edge(int u, int v, double w, double B, std::vector<double>& dist) {
  std::size_t uu = static_cast<std::size_t>(u);
  std::size_t vv = static_cast<std::size_t>(v);
  double cand = dist[uu] + w;
  if (cand <= dist[vv]) { bool improved = cand < dist[vv]; dist[vv] = cand; return improved && cand < B; }
  return false;
}
}

PivotResult FindPivots(const Graph& g, double B, const std::vector<int>& S, std::vector<double>& dist, const BMSSPParams& p) {
  std::vector<int> W = S;
  std::vector<int> layer_prev = S;
  std::vector<char> in_W(g.size(),0);
  for(int s:S) in_W[static_cast<std::size_t>(s)] = 1;
  const std::size_t k = p.k;
  for(std::size_t step=1; step<=k; ++step){
    std::vector<int> layer_curr;
    for(int u: layer_prev){
      for(const auto &e : g.neighbors(static_cast<std::size_t>(u))){
        if(relax_edge(u, static_cast<int>(e.to), e.w, B, dist)){
          if(!in_W[e.to]){ in_W[e.to]=1; layer_curr.push_back(static_cast<int>(e.to)); W.push_back(static_cast<int>(e.to)); }
        }
      }
    }
    layer_prev.swap(layer_curr);
    if(W.size() > k * S.size()) return {S,W};
  }
  std::vector<std::vector<int>> rev(g.size());
  std::vector<char> in_set(g.size(),0);
  for(int v:W) in_set[static_cast<std::size_t>(v)] = 1;
  for(int u:W){
    for(const auto &e : g.neighbors(static_cast<std::size_t>(u))){
      if(in_set[e.to] && dist[e.to] == dist[static_cast<std::size_t>(u)] + e.w)
        rev[static_cast<std::size_t>(u)].push_back(static_cast<int>(e.to));
    }
  }
  std::vector<int> P;
  for(int root : S){
    std::vector<int> stack{root};
    std::vector<int> order; order.reserve(64);
    std::unordered_map<int,std::size_t> it_index;
    while(!stack.empty()){
      int node = stack.back();
      std::size_t snode = static_cast<std::size_t>(node);
      if(it_index[node]==0) order.push_back(node);
      if(it_index[node] < rev[snode].size()){
        int child = rev[snode][it_index[node]++];
        if(child!=node) stack.push_back(child);
      } else stack.pop_back();
    }
    // Count; if at least k nodes reachable add root
    if(order.size() >= k) P.push_back(root);
  }
  return {P,W};
}

BMSSPResult BaseCase(double B, const std::vector<int>& S, const Graph& g, std::vector<double>& dist, const BMSSPParams& p) {
  int source = S.front();
  using Pair = std::pair<double,int>;
  std::priority_queue<Pair,std::vector<Pair>,std::greater<Pair>> pq;
  pq.emplace(dist[static_cast<std::size_t>(source)], source);
  std::vector<int> U0;
  const std::size_t limit = p.k + 1;
  while(!pq.empty() && U0.size() < limit){
    auto [du,u] = pq.top(); pq.pop();
    if(du != dist[static_cast<std::size_t>(u)]) continue;
    U0.push_back(u);
    for(const auto &e : g.neighbors(static_cast<std::size_t>(u))){
      double cand = dist[static_cast<std::size_t>(u)] + e.w;
      if(cand <= dist[e.to] && cand < B){
        if(cand < dist[e.to]){
          dist[e.to] = cand;
          pq.emplace(dist[e.to], static_cast<int>(e.to));
        }
      }
    }
  }
  if(U0.size() <= p.k) return {B,U0};
  double Bprime = 0.0;
  for(int v: U0) if(dist[static_cast<std::size_t>(v)] > Bprime) Bprime = dist[static_cast<std::size_t>(v)];
  std::vector<int> U;
  for(int v: U0) if(dist[static_cast<std::size_t>(v)] < Bprime) U.push_back(v);
  return {Bprime, U};
}

BMSSPResult BMSSP(int l, double B, const std::vector<int>& S, const Graph& g, std::vector<double>& dist, const BMSSPParams& p) {
  if(l==0) return BaseCase(B,S,g,dist,p);
  auto piv = FindPivots(g,B,S,dist,p);
  const auto &P = piv.P; const auto &W = piv.W;
  using Pair = std::pair<double,int>;
  std::priority_queue<Pair,std::vector<Pair>,std::greater<Pair>> D;
  for(int x: P) if(dist[static_cast<std::size_t>(x)] < B) D.emplace(dist[static_cast<std::size_t>(x)], x);
  double B0prime = (P.empty()? B : std::numeric_limits<double>::infinity());
  if(!P.empty()) for(int x: P) if(dist[static_cast<std::size_t>(x)] < B0prime) B0prime = dist[static_cast<std::size_t>(x)];
  std::vector<int> U;
  const std::size_t target = p.k * (std::size_t(1) << l) * p.t;
  while(U.size() < target && !D.empty()){
    auto [Bi, svert] = D.top(); D.pop();
    std::vector<int> Si{ svert };
    auto sub = BMSSP(l-1, Bi, Si, g, dist, p);
    U.insert(U.end(), sub.U.begin(), sub.U.end());
    for(int u: sub.U){
      for(const auto &e : g.neighbors(static_cast<std::size_t>(u))){
        double cand = dist[static_cast<std::size_t>(u)] + e.w;
        if(cand <= dist[e.to]){
          if(cand < dist[e.to]) dist[e.to] = cand;
          if(cand >= sub.boundary && cand < B) D.emplace(cand, static_cast<int>(e.to));
        }
      }
    }
  }
  double Bprime = B;
  for(int x: W) if(dist[static_cast<std::size_t>(x)] < Bprime) Bprime = dist[static_cast<std::size_t>(x)];
  std::vector<int> Uf;
  for(int x: W) if(dist[static_cast<std::size_t>(x)] < Bprime) Uf.push_back(x);
  U.insert(U.end(), Uf.begin(), Uf.end());
  return {Bprime, U};
}

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

} // namespace cpp_starter
