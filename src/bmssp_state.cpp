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
  if(u<0||v<0 || (size_t)u>=st.dist.size() || (size_t)v>=st.dist.size()) return false;
  auto& du = st.dist[u];
  auto& dv = st.dist[v];
  if(du.is_inf()) return false; // can't relax from infinity

  // Only supporting up to 64-bit active arithmetic initially; widening to 128 or bigint reserved for overflow.
  uint64_t du64 = du.as_u64_clamped();
  uint64_t cand;
  bool of = add_overflow_u64(du64, w, cand);
  if(of){
#if defined(ENABLE_DISTANCE_WIDENING)
    // Widen to 128 then retry (simplified placeholder)
    widen_word(st.dist[u], DistWidth::W128);
    widen_word(st.dist[v], DistWidth::W128);
    if(widened_out) *widened_out=true;
    // For now treat overflow as non-improvement (full 128-bit path TODO Phase 2b)
    return false;
#else
    throw std::overflow_error("Distance overflow and widening disabled");
#endif
  }
  bool better = cand < dv.as_u64_clamped();
  bool equal_ok = allow_equal && cand == dv.as_u64_clamped();
  if(!(better || equal_ok)) return false;
  if(better){
    // store cand
    if(dv.width==DistWidth::W32 && cand>std::numeric_limits<uint32_t>::max()) widen_word(dv, DistWidth::W64);
    if(dv.width==DistWidth::W64) dv.small.v64 = cand;
    else if(dv.width==DistWidth::W32) dv.small.v32 = (uint32_t)cand;
  }
  // hop/lexicographic tie-breaking not yet implemented (Phase 2 continuation)
  st.pred[v]=u;
  return better || equal_ok;
}

} // namespace bmssp
