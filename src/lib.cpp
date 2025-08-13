// License: Placeholder - add your license details here.
#include "cpp_starter/lib.hpp"
#include "cpp_starter/bmssp.hpp"
#include <queue>
#include <functional>
#include <unordered_map>

namespace cpp_starter {

int sum(int a, int b) { return a + b; }


// --- Simplified BMSSP Implementation ---

namespace {
// Helper to relax edge (u->v) if within bound B.
inline bool relax_edge(int u, int v, double w, double B, std::vector<double>& dist) {
	double cand = dist[u] + w;
	if (cand <= dist[v]) {
		bool improved = cand < dist[v];
		dist[v] = cand;
		return improved && cand < B;
	}
	return false;
}
}

PivotResult FindPivots(const Graph& g, double B, const std::vector<int>& S,
											 std::vector<double>& dist, const BMSSPParams& p) {
	// Initialize W with S
	std::vector<int> W = S;
	std::vector<int> layer_prev = S;
	std::vector<char> in_W(g.size(), 0);
	for (int s : S) in_W[s] = 1;

	// k from params
	std::size_t k = p.k;
	// Relax for k steps (Algorithm 1 lines 6-13)
	for (std::size_t step = 1; step <= k; ++step) {
		std::vector<int> layer_curr;
		for (int u : layer_prev) {
			for (auto& e : g.neighbors(u)) {
				if (relax_edge(u, e.to, e.w, B, dist)) {
					if (!in_W[e.to]) {
						in_W[e.to] = 1;
						layer_curr.push_back(e.to);
						W.push_back(e.to);
					}
				}
			}
		}
		layer_prev.swap(layer_curr);
		if (W.size() > k * S.size()) {
			// Early return P = S (line 15)
			return PivotResult{S, W};
		}
	}

	// Build forest F: edges (u,v) with u,v in W and dist[v] == dist[u] + w_uv
	// We'll approximate subtree sizes via reverse adjacency.
	std::vector<std::vector<int>> rev(g.size());
	std::vector<char> in_set(g.size(), 0);
	for (int v : W) in_set[v] = 1;
	for (int u : W) {
		for (auto& e : g.neighbors(u)) {
			if (in_set[e.to] && dist[e.to] == dist[u] + e.w) {
				rev[u].push_back(e.to); // u -> child v
			}
		}
	}

	// Compute subtree sizes with DFS starting from each S member, limiting to k.
	std::vector<int> subtree(g.size(), 0);
	std::vector<int> P;
	for (int root : S) {
		// Iterative DFS
		std::vector<int> stack{root};
		std::vector<int> order; order.reserve(64);
		std::unordered_map<int, std::size_t> it_index;
		while (!stack.empty()) {
			int node = stack.back();
			if (it_index[node] == 0) {
				order.push_back(node);
			}
			if (it_index[node] < rev[node].size()) {
				int child = rev[node][it_index[node]++];
				if (child != node) stack.push_back(child);
			} else {
				stack.pop_back();
			}
		}
		int count = 0;
		for (int v : order) {
			if (++count >= static_cast<int>(k)) break; // early cutoff
		}
		if (count >= static_cast<int>(k)) {
			P.push_back(root);
		}
	}

	return PivotResult{P, W};
}

BMSSPResult BaseCase(double B, const std::vector<int>& S, const Graph& g,
										 std::vector<double>& dist, const BMSSPParams& p) {
	// Precondition: |S| == 1
	int source = S.front();
	// Min-heap (distance, vertex)
	using Pair = std::pair<double,int>;
	std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> pq;
	pq.emplace(dist[source], source);
	std::vector<int> U0;
	std::vector<char> in_heap(g.size(), 0);
	in_heap[source] = 1;
	std::size_t limit = p.k + 1;
	while (!pq.empty() && U0.size() < limit) {
		auto [du,u] = pq.top(); pq.pop();
		if (du != dist[u]) continue;
		U0.push_back(u);
		for (auto& e : g.neighbors(u)) {
			double cand = dist[u] + e.w;
			if (cand <= dist[e.to] && cand < B) {
				if (cand < dist[e.to]) dist[e.to] = cand;
				pq.emplace(dist[e.to], e.to);
			}
		}
	}
	if (U0.size() <= p.k) {
		return {B, U0};
	} else {
		// boundary is max dist in U0
		double Bprime = 0.0;
		for (int v : U0) if (dist[v] > Bprime) Bprime = dist[v];
		std::vector<int> U;
		U.reserve(U0.size());
		for (int v : U0) if (dist[v] < Bprime) U.push_back(v);
		return {Bprime, U};
	}
}

BMSSPResult BMSSP(int l, double B, const std::vector<int>& S, const Graph& g,
									std::vector<double>& dist, const BMSSPParams& p) {
	if (l == 0) {
		return BaseCase(B, S, g, dist, p);
	}
	auto piv = FindPivots(g, B, S, dist, p);
	const auto& P = piv.P;
	const auto& W = piv.W;
	// Data structure 𝒟 approximated with min-heap of (distance, vertex)
	using Pair = std::pair<double,int>;
	std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> D;
	for (int x : P) {
		if (dist[x] < B) D.emplace(dist[x], x);
	}
	double B0prime = (P.empty() ? B : std::numeric_limits<double>::infinity());
	if (!P.empty()) {
		for (int x : P) if (dist[x] < B0prime) B0prime = dist[x];
	}
	std::vector<int> U;
	std::size_t target = p.k * (1u << l) * p.t; // heuristic substitute for k * 2^l * t
	int iter = 0;
	while (U.size() < target && !D.empty()) {
		++iter;
		auto [Bi, Si_vertex] = D.top(); D.pop();
		std::vector<int> Si = {Si_vertex};
		auto sub = BMSSP(l - 1, Bi, Si, g, dist, p);
		U.insert(U.end(), sub.U.begin(), sub.U.end());
		// Explore edges from new vertices (line 18 onward simplified)
		for (int u : sub.U) {
			for (auto& e : g.neighbors(u)) {
				double cand = dist[u] + e.w;
				if (cand <= dist[e.to]) {
					if (cand < dist[e.to]) dist[e.to] = cand;
					if (cand >= sub.boundary && cand < B) {
						D.emplace(cand, e.to);
					}
				}
			}
		}
	}
	// Final boundary: min of recursive boundaries and B (simplified)
	double Bprime = B;
	for (int x : W) if (dist[x] < Bprime) Bprime = dist[x];
	std::vector<int> Ufiltered;
	for (int x : W) if (dist[x] < Bprime) Ufiltered.push_back(x);
	// Combine previously gathered U with filtered W
	U.insert(U.end(), Ufiltered.begin(), Ufiltered.end());
	return {Bprime, U};
}

}  // namespace cpp_starter
