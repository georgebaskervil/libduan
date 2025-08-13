#include "cpp_starter/bmssp/state.hpp"
#include <limits>
#if defined(__has_builtin)
#  if __has_builtin(__builtin_add_overflow)
#    define BMSSP_HAVE_ADD_OVERFLOW 1
#  endif
#endif

namespace bmssp {

static bool add_overflow_u64(uint64_t a, uint64_t b, uint64_t& out){
#if BMSSP_HAVE_ADD_OVERFLOW
  return __builtin_add_overflow(a,b,&out);
#else
  unsigned __int128 sum=(unsigned __int128)a + b; out=(uint64_t)sum; return sum>std::numeric_limits<uint64_t>::max();
#endif
}

static void widen_word(DistWord& w, DistWidth target){
  if(w.width==target) return;
  if(target==DistWidth::W64 && w.width==DistWidth::W32){ w.small.v64 = w.small.v32; w.width=DistWidth::W64; return; }
  if(target==DistWidth::W128){
    unsigned __int128 val=0;
    if(w.width==DistWidth::W32) val = w.small.v32;
    else if(w.width==DistWidth::W64) val = w.small.v64;
    else if(w.width==DistWidth::W128) return;
    w.small.v128 = val; w.width=DistWidth::W128; return;
  }
#ifdef ENABLE_BIGINT_FALLBACK
  if(target==DistWidth::BIG){
    uint64_t low = w.as_u64_clamped();
    w.big.assign(low); w.width=DistWidth::BIG; return;
  }
#endif
}

bool relax(DistState& st, int u, int v, uint64_t w, bool allow_equal, int vertex_id_tiebreak, bool* widened_out){
  if(widened_out) *widened_out=false;
  if(u<0||v<0) return false;
  std::size_t su = static_cast<std::size_t>(u);
  std::size_t sv = static_cast<std::size_t>(v);
  if(su>=st.dist.size() || sv>=st.dist.size()) return false;
  auto& du = st.dist[su];
  auto& dv = st.dist[sv];
  if(du.is_inf()) return false; // can't relax from infinity

  // Attempt addition in current width (<=64 bits fast path) else promote.
  unsigned __int128 cand128=0;
  bool better=false, equal_dist=false;
  uint64_t old_v = dv.as_u64_clamped();
  if(du.width==DistWidth::W32 || du.width==DistWidth::W64){
    uint64_t base_u = du.as_u64_clamped();
    uint64_t cand64;
    bool of = add_overflow_u64(base_u, w, cand64);
    if(of){
#ifdef ENABLE_DISTANCE_WIDENING
  widen_word(st.dist[su], DistWidth::W128);
  widen_word(st.dist[sv], DistWidth::W128);
#ifdef ENABLE_BMSSP_VERIFIER
  st.widen_events++; st.overflow_events++;
#endif
  if(widened_out) *widened_out=true;
  cand128 = (unsigned __int128)base_u + w; // 128-bit exact
#else
  throw std::overflow_error("Distance overflow and widening disabled");
#endif // ENABLE_DISTANCE_WIDENING
    } else {
  cand128 = cand64;
    }
  } else if(du.width==DistWidth::W128){
    cand128 = du.small.v128 + (unsigned __int128)w; // rely on native 128-bit wrap avoidance
  } else { // BIG
#ifdef ENABLE_BIGINT_FALLBACK
    // BIGINT add: approximate using lower 128 bits for comparison path; full big add not yet needed since w fits in 64.
    uint64_t base_low = du.big.limbs.empty()?0:du.big.limbs[0];
    unsigned __int128 base_high = 0;
    if(du.big.limbs.size()>1) base_high = (unsigned __int128)du.big.limbs[1] << 64;
    unsigned __int128 base128 = base_high | base_low;
    cand128 = base128 + (unsigned __int128)w;
#else
    throw std::overflow_error("BIGINT arithmetic requested but fallback disabled");
#endif
  }
  better = (cand128 < old_v);
  equal_dist = (cand128 == old_v);
  if(!(better || (allow_equal && equal_dist))) return false;

  // Lexicographic tie-breaking: (distance, hop, vertex_id) vs existing (old_dist, old_hop, pred_vertex_id)
  int new_hop = (st.hop[su] < std::numeric_limits<int>::max()) ? st.hop[su] + 1 : st.hop[su];
  bool take = better;
  if(!take && equal_dist && allow_equal){
    int old_hop = st.hop[sv];
    if(new_hop < old_hop) take = true;
    else if(new_hop == old_hop){
      // use vertex_id_tiebreak (prefer smaller id if provided else source u)
      int chosen_id = vertex_id_tiebreak==0? u : vertex_id_tiebreak;
      int old_pred = st.pred[sv];
      if(old_pred<0 || chosen_id < old_pred) take = true;
    }
  }
  if(!take) return false;

  // Store candidate with appropriate width
  if(cand128 <= std::numeric_limits<uint32_t>::max()){
    if(dv.width!=DistWidth::W32) dv.width=DistWidth::W32;
    dv.small.v32 = (uint32_t)cand128;
  } else if(cand128 <= std::numeric_limits<uint64_t>::max()){
    if(dv.width==DistWidth::W32) dv.width=DistWidth::W64;
    if(dv.width==DistWidth::W64) dv.small.v64 = (uint64_t)cand128;
    else if(dv.width==DistWidth::W128) dv.small.v128 = cand128; // fits (value within 64)
  } else {
    widen_word(dv, DistWidth::W128);
    dv.small.v128 = cand128; // may exceed 64-bit
  }
  st.pred[sv] = u;
  st.hop[sv] = new_hop;
  return true;
}

} // namespace bmssp
