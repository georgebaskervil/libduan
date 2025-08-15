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
  static BigInt add_u64(const BigInt& a, uint64_t b){
    BigInt r; r.limbs.resize(a.limbs.size()+1,0);
    unsigned __int128 carry=b; std::size_t i=0; for(; i<a.limbs.size(); ++i){ unsigned __int128 sum = (unsigned __int128)a.limbs[i] + carry; r.limbs[i] = (uint64_t)sum; carry = sum >> 64; }
    r.limbs[i] = (uint64_t)carry; if(r.limbs.back()==0) r.limbs.pop_back(); return r; }
  void add_u64_inplace(uint64_t b){ unsigned __int128 carry=b; std::size_t i=0; for(; i<limbs.size() && carry; ++i){ unsigned __int128 sum=(unsigned __int128)limbs[i]+carry; limbs[i]=(uint64_t)sum; carry=sum>>64; } if(carry) limbs.push_back((uint64_t)carry); }
  static int cmp(const BigInt& a, const BigInt& b){
    if(a.limbs.size()!=b.limbs.size()) return a.limbs.size()<b.limbs.size() ? -1:1;
    for(std::size_t i=a.limbs.size(); i-- > 0;){ if(a.limbs[i]!=b.limbs[i]) return a.limbs[i]<b.limbs[i]? -1:1; }
    return 0;
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

inline BigInt distword_to_big(const DistWord& d){
  BigInt b; switch(d.width){
    case DistWidth::W32: b.assign(d.small.v32); break;
    case DistWidth::W64: b.assign(d.small.v64); break;
    case DistWidth::W128: b.assign_u128(d.small.v128); break;
    case DistWidth::BIG: return d.big; }
  return b;
}

// Compare a DistWord (which may itself be BIG) against a BigInt candidate value.
// Returns -1 if cand < current, 0 if equal, 1 if cand > current.
inline int compare_distword_big_candidate(const DistWord& current, const BigInt& cand){
  if(current.width!=DistWidth::BIG){
    BigInt cur = distword_to_big(current);
    int c = BigInt::cmp(cur, cand);
    if(c<0) return 1; // cand greater
    if(c>0) return -1; // cand smaller
    return 0;
  }
  int c = BigInt::cmp(current.big, cand);
  if(c<0) return 1; // cand greater
  if(c>0) return -1; // cand smaller
  return 0;
}

// Total order compare between two DistWords (handles BIG precisely).
inline int compare(const DistWord& a, const DistWord& b){
  if(a.width==DistWidth::BIG || b.width==DistWidth::BIG){
#ifdef ENABLE_BIGINT_FALLBACK
    BigInt ab = (a.width==DistWidth::BIG)? a.big : distword_to_big(a);
    BigInt bb = (b.width==DistWidth::BIG)? b.big : distword_to_big(b);
    return BigInt::cmp(ab,bb);
#else
    // Fallback disabled: compare truncated 128-bit representations
    unsigned __int128 av=0,bv=0; if(a.width==DistWidth::W32) av=a.small.v32; else if(a.width==DistWidth::W64) av=a.small.v64; else if(a.width==DistWidth::W128) av=a.small.v128; if(b.width==DistWidth::W32) bv=b.small.v32; else if(b.width==DistWidth::W64) bv=b.small.v64; else if(b.width==DistWidth::W128) bv=b.small.v128; if(av<bv) return -1; if(av>bv) return 1; return 0;
#endif
  }
  // Neither BIG
  if(a.width!=b.width){
    unsigned __int128 av=0,bv=0;
    if(a.width==DistWidth::W32) av=a.small.v32; else if(a.width==DistWidth::W64) av=a.small.v64; else av=a.small.v128;
    if(b.width==DistWidth::W32) bv=b.small.v32; else if(b.width==DistWidth::W64) bv=b.small.v64; else bv=b.small.v128;
    if(av<bv) return -1; if(av>bv) return 1; return 0;
  }
  switch(a.width){
    case DistWidth::W32: if(a.small.v32<b.small.v32) return -1; if(a.small.v32>b.small.v32) return 1; return 0;
    case DistWidth::W64: if(a.small.v64<b.small.v64) return -1; if(a.small.v64>b.small.v64) return 1; return 0;
    case DistWidth::W128: if(a.small.v128<b.small.v128) return -1; if(a.small.v128>b.small.v128) return 1; return 0;
    case DistWidth::BIG: /* handled above */ return 0;
  }
  return 0;
}

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
  uint64_t relax_improve_events=0;   // strict improvement
  uint64_t relax_equal_events=0;     // accepted equal-distance relaxation
  uint64_t relax_reject_events=0;    // rejected attempts (allow_equal true but tie-break lost)
  uint64_t relax_attempts=0;         // total relax calls reaching comparison stage
  uint64_t oom_simulated_events=0;   // simulated OOM occurrences
  // Additional tracking
  uint64_t widen_pairs=0;            // number of times both endpoints widened together
  uint64_t bigint_total_limbs=0;     // cumulative limbs assigned during BIG promotions
  uint64_t widen_bytes=0;            // theoretical bytes added by widening (approx)
  uint64_t big_bytes=0;              // bytes in BigInt limbs assigned
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
bool relax(DistState& st, int u, int v, uint64_t w, bool allow_equal=true, bool* widened_out=nullptr);

// Optional hook to simulate OOM during widening for testing failure semantics.
inline bool simulate_oom(){
#ifdef BMSSP_SIMULATE_OOM
  return true;
#else
  return false;
#endif
}

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

// Removed old truncated compare_candidate_u128 (superseded by full compare functions)



} // namespace bmssp
