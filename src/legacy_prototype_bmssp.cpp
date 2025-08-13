#include "cpp_starter/bmssp.hpp"
#include <queue>
#include <functional>
#include <unordered_map>

namespace cpp_starter {
// (Legacy prototype implementation retained verbatim from previous lib.cpp)
namespace {
inline bool relax_edge(int u, int v, double w, double B, std::vector<double>& dist) {
  double cand = dist[u] + w;
  if (cand <= dist[v]) { bool improved = cand < dist[v]; dist[v] = cand; return improved && cand < B; }
  return false;
}
}

PivotResult FindPivots(const Graph& g, double B, const std::vector<int>& S, std::vector<double>& dist, const BMSSPParams& p) {
  std::vector<int> W = S; std::vector<int> layer_prev = S; std::vector<char> in_W(g.size(),0); for(int s:S) in_W[s]=1; size_t k=p.k;
  for(size_t step=1; step<=k; ++step){ std::vector<int> layer_curr; for(int u:layer_prev){ for(auto&e:g.neighbors(u)){ if(relax_edge(u,e.to,e.w,B,dist)){ if(!in_W[e.to]){ in_W[e.to]=1; layer_curr.push_back(e.to); W.push_back(e.to);} } } } layer_prev.swap(layer_curr); if(W.size()>k*S.size()) return {S,W}; }
  std::vector<std::vector<int>> rev(g.size()); std::vector<char> in_set(g.size(),0); for(int v:W) in_set[v]=1; for(int u:W){ for(auto&e:g.neighbors(u)){ if(in_set[e.to] && dist[e.to]==dist[u]+e.w) rev[u].push_back(e.to);} }
  std::vector<int> P; for(int root:S){ std::vector<int> stack{root}; std::vector<int> order; order.reserve(64); std::unordered_map<int,size_t> it_index; while(!stack.empty()){ int node=stack.back(); if(it_index[node]==0) order.push_back(node); if(it_index[node]<rev[node].size()){ int child=rev[node][it_index[node]++]; if(child!=node) stack.push_back(child);} else stack.pop_back(); } int count=0; for(int v:order){ if(++count >= (int)k) break;} if(count >= (int)k) P.push_back(root);} return {P,W}; }

BMSSPResult BaseCase(double B, const std::vector<int>& S, const Graph& g, std::vector<double>& dist, const BMSSPParams& p) {
  int source=S.front(); using Pair=std::pair<double,int>; std::priority_queue<Pair,std::vector<Pair>,std::greater<Pair>> pq; pq.emplace(dist[source],source); std::vector<int> U0; size_t limit=p.k+1; while(!pq.empty() && U0.size()<limit){ auto [du,u]=pq.top(); pq.pop(); if(du!=dist[u]) continue; U0.push_back(u); for(auto&e:g.neighbors(u)){ double cand=dist[u]+e.w; if(cand<=dist[e.to] && cand < B){ if(cand<dist[e.to]) dist[e.to]=cand; pq.emplace(dist[e.to], e.to); } } } if(U0.size()<=p.k) return {B,U0}; double Bprime=0.0; for(int v:U0) if(dist[v]>Bprime) Bprime=dist[v]; std::vector<int> U; for(int v:U0) if(dist[v]<Bprime) U.push_back(v); return {Bprime,U}; }

BMSSPResult BMSSP(int l, double B, const std::vector<int>& S, const Graph& g, std::vector<double>& dist, const BMSSPParams& p) {
  if(l==0) return BaseCase(B,S,g,dist,p); auto piv=FindPivots(g,B,S,dist,p); const auto&P=piv.P; const auto&W=piv.W; using Pair=std::pair<double,int>; std::priority_queue<Pair,std::vector<Pair>,std::greater<Pair>> D; for(int x:P) if(dist[x]<B) D.emplace(dist[x],x); double B0prime=(P.empty()?B:std::numeric_limits<double>::infinity()); if(!P.empty()) for(int x:P) if(dist[x]<B0prime) B0prime=dist[x]; std::vector<int> U; size_t target=p.k*(1u<<l)*p.t; while(U.size()<target && !D.empty()){ auto [Bi, svert]=D.top(); D.pop(); std::vector<int> Si={svert}; auto sub=BMSSP(l-1,Bi,Si,g,dist,p); U.insert(U.end(),sub.U.begin(),sub.U.end()); for(int u:sub.U){ for(auto&e:g.neighbors(u)){ double cand=dist[u]+e.w; if(cand<=dist[e.to]){ if(cand<dist[e.to]) dist[e.to]=cand; if(cand>=sub.boundary && cand<B) D.emplace(cand,e.to); } } } } double Bprime=B; for(int x:W) if(dist[x]<Bprime) Bprime=dist[x]; std::vector<int> Uf; for(int x:W) if(dist[x]<Bprime) Uf.push_back(x); U.insert(U.end(),Uf.begin(),Uf.end()); return {Bprime,U}; }

} // namespace cpp_starter
