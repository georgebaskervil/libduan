#pragma once
#include <vector>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <cstring>

namespace bmssp {

enum class DistWidth : uint8_t { W32=0, W64=1, W128=2, BIG=3 };

struct BigInt {
  // little-endian 64-bit limbs
  std::vector<uint64_t> limbs;
  void assign(uint64_t v) { limbs.assign(1, v); }
  static BigInt add(const BigInt& a, const BigInt& b, bool& overflow) {
    BigInt r; r.limbs.resize(std::max(a.limbs.size(), b.limbs.size())+1,0);
    unsigned __int128 carry=0; overflow=false;
    for(size_t i=0;i<r.limbs.size();++i){ unsigned __int128 sum=carry; if(i<a.limbs.size()) sum+=a.limbs[i]; if(i<b.limbs.size()) sum+=b.limbs[i]; r.limbs[i]=(uint64_t)sum; carry=sum>>64; }
    if(r.limbs.back()==0) r.limbs.pop_back(); else if(carry) overflow=false; return r;
  }
};

struct DistWord {
  DistWidth width{DistWidth::W64};
  union { uint32_t v32; uint64_t v64; unsigned __int128 v128; } small{};
  BigInt big; // used only if width==BIG

  static DistWord inf() {
    DistWord d; d.width=DistWidth::W64; d.small.v64=UINT64_MAX; return d; }
  bool is_inf() const { return width==DistWidth::W64 && small.v64==UINT64_MAX; }
  uint64_t as_u64_clamped() const {
    switch(width){
      case DistWidth::W32: return small.v32;
      case DistWidth::W64: return small.v64;
      case DistWidth::W128: return (uint64_t)small.v128; // potential truncation for diagnostics only
      case DistWidth::BIG: return big.limbs.empty()?0:big.limbs[0];
    }
    return 0;
  }
};

struct DistState {
  std::vector<DistWord> dist;
  std::vector<int> pred;
  std::vector<int> hop;
  std::vector<uint8_t> complete;
  DistWidth active_width{DistWidth::W64};
  bool used_bigint=false;
};

inline DistState make_state(std::size_t n) {
  DistState s; s.dist.assign(n, DistWord::inf()); s.pred.assign(n,-1); s.hop.assign(n,0); s.complete.assign(n,0); return s; }

// Overflow-safe addition with widening; returns true if improved.
bool relax(DistState& st, int u, int v, uint64_t w, bool allow_equal=true, int vertex_id_tiebreak=0, bool* widened_out=nullptr);

} // namespace bmssp
