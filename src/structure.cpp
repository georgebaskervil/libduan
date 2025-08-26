// Phase 4: Data structure 𝒟 (Lemma 3.3) implementation
// This implementation focuses on correctness and a clear API match.
// It maintains two sequences:
//  - D0: sequence of blocks added by batch_prepend, each with up to M items, sorted by value
//  - D1: map from upper_bound (value) to a vector of singleton items from insert()
// A best_ map tracks the current minimal value per key to provide decrease-key semantics
// and to filter stale entries during pulls.

#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <queue>
#include <unordered_map>

#include "cpp_starter/bmssp/structure.hpp"

namespace bmssp {

namespace {
inline unsigned __int128 max_u128(unsigned __int128 a, unsigned __int128 b) {
  return a > b ? a : b;
}
}  // namespace

class PartialPriorityImpl {
 public:
  void initialize(std::size_t M, unsigned __int128 B) {
    M_ = (M == 0 ? 1 : M);
    B_ = B;
    best_.clear();
  while (!heap_.empty()) heap_.pop();
  }

  void insert(int key, unsigned __int128 value) {
    auto it = best_.find(key);
    if (it != best_.end()) {
      if (value >= it->second) return;  // not an improvement
      it->second = value;
    } else {
      best_.emplace(key, value);
    }
  // push into min-heap
  heap_.push(PartialPriority::Item{key, value});
  }

  void batch_prepend(std::vector<PartialPriority::Item>& batch) {
    if (batch.empty()) return;
    // Apply decrease-key filtering against best_ and collect survivors
    std::vector<PartialPriority::Item> filtered;
    filtered.reserve(batch.size());
    for (const auto& it : batch) {
      auto f = best_.find(it.key);
      if (f == best_.end() || it.value < f->second) {
        best_[it.key] = it.value;
        filtered.push_back(it);
      }
    }
    if (filtered.empty()) return;
    // Sort by ascending value
    std::sort(filtered.begin(), filtered.end(), [](const auto& a, const auto& b) {
      if (a.value != b.value) return a.value < b.value;
      return a.key < b.key;
    });
    // Insert into D1 buckets; map keeps global order by value
    for (const auto& it : filtered) {
      heap_.push(it);
    }
  }

  bool pull(std::vector<int>& out_keys, unsigned __int128& boundary) {
    out_keys.clear();
    // Fast check: if no live keys, return false
    if (best_.empty()) return false;

    auto peek_heap = [this](PartialPriority::Item& out) -> bool {
      if (heap_.empty()) return false;
      out = heap_.top();
      return true;
    };

    auto consume_heap = [this]() {
      if (heap_.empty()) return;
      heap_.pop();
    };

    unsigned __int128 max_val = 0;
    out_keys.reserve(M_);

    while (out_keys.size() < M_) {
  PartialPriority::Item pick;
  bool hb = peek_heap(pick);
      if (!hb) break;

      // Check if this item is still the best for its key (skip stale)
      auto itb = best_.find(pick.key);
      if (itb == best_.end() || itb->second != pick.value) {
        // stale entry, just consume and continue without emitting
  consume_heap();
        continue;
      }

      // Emit this key and remove from structure (so it won't be returned again)
      out_keys.push_back(pick.key);
      max_val = max_u128(max_val, pick.value);
      best_.erase(itb);
  consume_heap();
    }

    if (out_keys.empty()) return false;
    boundary = max_val;
    return true;
  }

  bool empty() const noexcept {
    return best_.empty();
  }

 private:
  std::size_t M_ = 0;
  unsigned __int128 B_ = 0;
  std::unordered_map<int, unsigned __int128> best_;
  struct Cmp {
    bool operator()(const PartialPriority::Item& a, const PartialPriority::Item& b) const {
      if (a.value != b.value) return a.value > b.value;  // min-heap by value
      return a.key > b.key;                               // tie-break by key
    }
  };
  std::priority_queue<PartialPriority::Item, std::vector<PartialPriority::Item>, Cmp> heap_;
};

// Thin wrappers mapping header API to the implementation class, without exposing it in the header
namespace {
// Store one impl per process; the class itself contains no global state that depends on graphs.
// The header declares a value-type class, so we embed an impl instance inside it by pointer.
struct Holder {
  PartialPriorityImpl impl;
};
}  // namespace

// To avoid changing the public header, we store the impl in a static map keyed by this pointer.
// Given the scope of this workspace, we can instead embed as a function-static map, but
// here we’ll attach an impl per object by placement in an aligned storage via a pointer field
// simulated with a reinterpret_cast using a private buffer in the header. Since the header
// doesn’t provide such a buffer, we keep a map. Simpler: use a function-static unordered_map.

static std::unordered_map<const PartialPriority*, Holder>& get_registry() {
  static std::unordered_map<const PartialPriority*, Holder> reg;
  return reg;
}

void PartialPriority::initialize(std::size_t M, unsigned __int128 B) {
  auto& reg = get_registry();
  auto& h = reg[this];
  h.impl.initialize(M, B);
  M_ = M;
  B_ = B;
  size_ = 0;  // not used; empty() delegates to impl
}

void PartialPriority::insert(int key, unsigned __int128 value) {
  auto& h = get_registry()[this];
  h.impl.insert(key, value);
}

void PartialPriority::batch_prepend(std::vector<Item>& batch) {
  auto& h = get_registry()[this];
  h.impl.batch_prepend(batch);
}

bool PartialPriority::pull(std::vector<int>& out_keys, unsigned __int128& boundary) {
  auto& h = get_registry()[this];
  return h.impl.pull(out_keys, boundary);
}

bool __pp_empty(const PartialPriority* self) noexcept {
  auto& reg = get_registry();
  auto it = reg.find(self);
  if (it == reg.end()) return true;
  return it->second.impl.empty();
}

}  // namespace bmssp
