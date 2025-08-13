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
  void assign_u128(unsigned __int128 v){
    limbs.clear();
    uint64_t lo = static_cast<uint64_t>(v);
    uint64_t hi = static_cast<uint64_t>(v >> 64);
    limbs.push_back(lo);
    if(hi) limbs.push_back(hi);
  }
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
  static DistWord from_u64(uint64_t v){ DistWord d; if(v<=std::numeric_limits<uint32_t>::max()){ d.width=DistWidth::W32; d.small.v32=(uint32_t)v; } else { d.width=DistWidth::W64; d.small.v64=v; } return d; }
  static DistWord from_u128(unsigned __int128 v){ DistWord d; if(v<=std::numeric_limits<uint64_t>::max()){ d.width=DistWidth::W64; d.small.v64=(uint64_t)v; } else { d.width=DistWidth::W128; d.small.v128=v; } return d; }
  void assign_u64(uint64_t v){ if(v<=std::numeric_limits<uint32_t>::max()){ width=DistWidth::W32; small.v32=(uint32_t)v; } else { width=DistWidth::W64; small.v64=v; } }
  void assign_u128(unsigned __int128 v){ if(v<=std::numeric_limits<uint64_t>::max()) assign_u64((uint64_t)v); else { width=DistWidth::W128; small.v128=v; } }
};

struct DistState {
  std::vector<DistWord> dist;
  std::vector<int> pred;
  std::vector<int> hop;
  std::vector<uint8_t> complete;
  DistWidth active_width{DistWidth::W64};
  bool used_bigint=false;
#ifdef ENABLE_BMSSP_VERIFIER
  // Diagnostics counters (incremented in relax / widening paths)
  uint64_t widen_events=0;
  uint64_t overflow_events=0;
#endif
};

inline DistState make_state(std::size_t n) {
  DistState s; 
#if MIN_DISTANCE_BITS==32
  s.active_width = DistWidth::W32;
  DistWord inf32; inf32.width=DistWidth::W32; inf32.small.v32=UINT32_MAX; s.dist.assign(n, inf32);
#else
  s.active_width = DistWidth::W64;
  s.dist.assign(n, DistWord::inf());
#endif
  s.pred.assign(n,-1); s.hop.assign(n,0); s.complete.assign(n,0); return s; }

// Mark vertex complete
inline void mark_complete(DistState& st, int v){ if(v>=0){ std::size_t sv = static_cast<std::size_t>(v); if(sv<st.complete.size()) st.complete[sv]=1; } }

// Initialize sources S (set distance 0, predecessor self, hop=0, complete=1)
inline void initialize_sources(DistState& st, const std::vector<int>& S){
  for(int s : S){ if(s<0) continue; std::size_t ss = static_cast<std::size_t>(s); if(ss>=st.dist.size()) continue; auto &dw = st.dist[ss];
    if(dw.width==DistWidth::W32) dw.small.v32 = 0; else if(dw.width==DistWidth::W64) dw.small.v64 = 0; else if(dw.width==DistWidth::W128) dw.small.v128 = 0; else if(dw.width==DistWidth::BIG) dw.big.assign(0);
    st.pred[ss]=s; st.hop[ss]=0; st.complete[ss]=1; }
}

// Return true if vertex is complete
inline bool is_complete(const DistState& st, int v){ return v>=0 && static_cast<std::size_t>(v) < st.complete.size() && st.complete[static_cast<std::size_t>(v)]!=0; }

// Overflow-safe addition with widening; returns true if improved.
bool relax(DistState& st, int u, int v, uint64_t w, bool allow_equal=true, int vertex_id_tiebreak=0, bool* widened_out=nullptr);

// Helper: promote a DistWord to BIG (BigInt) form preserving value.
inline void promote_to_big(DistWord& dw){
#ifdef ENABLE_BIGINT_FALLBACK
  if(dw.width==DistWidth::BIG) return;
  if(dw.width==DistWidth::W32){ dw.big.assign(dw.small.v32); }
  else if(dw.width==DistWidth::W64){ dw.big.assign(dw.small.v64); }
  else if(dw.width==DistWidth::W128){ dw.big.assign_u128(dw.small.v128); }
  dw.width = DistWidth::BIG;
#else
  (void)dw; // suppression
  throw std::overflow_error("BigInt fallback disabled");
#endif
}

// Compare DistWord (possibly BIG) with a candidate unsigned __int128 value.
// Returns -1 if candidate < current, 0 if equal, 1 if candidate > current.
inline int compare_candidate_u128(const DistWord& current, unsigned __int128 cand){
  switch(current.width){
    case DistWidth::W32: {
      unsigned __int128 cur = current.small.v32; if(cand<cur) return -1; if(cand>cur) return 1; return 0; }
    case DistWidth::W64: {
      unsigned __int128 cur = current.small.v64; if(cand<cur) return -1; if(cand>cur) return 1; return 0; }
    case DistWidth::W128: {
      unsigned __int128 cur = current.small.v128; if(cand<cur) return -1; if(cand>cur) return 1; return 0; }
    case DistWidth::BIG: {
#ifdef ENABLE_BIGINT_FALLBACK
      // Interpret first two limbs (if present) as 128-bit truncated value.
      unsigned __int128 cur = 0;
      if(!current.big.limbs.empty()){
        cur = current.big.limbs[0];
        if(current.big.limbs.size()>1){ cur |= (unsigned __int128)current.big.limbs[1] << 64; }
      }
      // If BIG has more than 2 limbs, it is definitely > 2^128-1 so cand < current unless cand represents same truncated and higher limbs are zero (impossible). Treat as cand<current.
      if(current.big.limbs.size()>2) return -1;
      if(cand<cur) return -1; if(cand>cur) return 1; return 0;
#else
      // Fallback disabled, treat as equal for safety.
      return 0;
#endif
    }
  }
  return 0;
}



} // namespace bmssp
