#pragma once
#include <cstdint>
#include <utility>
#include <vector>

namespace bmssp {

// Interface skeleton for data structure D (Lemma 3.3)
class PartialPriority {
 public:
  struct Item {
    int key;
    unsigned __int128 value;
  };
  void initialize(std::size_t M, unsigned __int128 B);
  void insert(int key, unsigned __int128 value);  // Insert / decrease-key semantics
  void batch_prepend(std::vector<Item>& batch);   // values all smaller than any existing
  bool pull(std::vector<int>& out_keys, unsigned __int128& boundary);  // returns up to M keys
  bool empty() const noexcept;  // defined inline below via helper

 private:
  std::size_t M_ = 0;
  unsigned __int128 B_ = 0;
  std::size_t size_ = 0;  // number of keys stored
};

// Helper to avoid exposing implementation details in header
bool __pp_empty(const PartialPriority* self) noexcept;

inline bool PartialPriority::empty() const noexcept {
  return __pp_empty(this);
}

}  // namespace bmssp
