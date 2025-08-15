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

bool relax(DistState& st, int u, int v, uint64_t w, bool allow_equal, bool* widened_out){
  if(widened_out) *widened_out=false;
  if(u<0||v<0) return false;
  const std::size_t su = static_cast<std::size_t>(u);
  const std::size_t sv = static_cast<std::size_t>(v);
  if(su>=st.dist.size() || sv>=st.dist.size()) return false;
  DistWord& du = st.dist[su];
  DistWord& dv = st.dist[sv];
  if(du.is_inf()) return false; // can't relax from infinity
  bool dv_was_inf = dv.is_inf();
#ifdef ENABLE_BMSSP_VERIFIER
  st.relax_attempts++;
#endif

  enum class CandKind { K128, KBIG };
  CandKind kind = CandKind::K128;
  unsigned __int128 cand128 = 0;
  BigInt candBig;

  auto width_bytes = [](DistWidth w)->uint64_t{
    switch(w){ case DistWidth::W32: return 4; case DistWidth::W64: return 8; case DistWidth::W128: return 16; case DistWidth::BIG: return 0; }
    return 0; };

  // Form candidate
  if(du.width==DistWidth::W32 || du.width==DistWidth::W64){
    uint64_t base_u = du.as_u64_clamped();
    uint64_t cand64;
    bool of = add_overflow_u64(base_u, w, cand64);
    if(of){
#ifdef ENABLE_DISTANCE_WIDENING
      if(simulate_oom()){
#ifdef ENABLE_BMSSP_VERIFIER
        st.oom_simulated_events++;
#endif
        throw std::runtime_error("Simulated OOM during widening");
      }
  // track bytes widened for both endpoints
#ifdef ENABLE_BMSSP_VERIFIER
  uint64_t u_before = width_bytes(st.dist[su].width);
  uint64_t v_before = width_bytes(st.dist[sv].width);
#endif
      widen_word(st.dist[su], DistWidth::W128);
      widen_word(st.dist[sv], DistWidth::W128);
#ifdef ENABLE_BMSSP_VERIFIER
      st.widen_events++; st.overflow_events++; st.widen_pairs++;
  st.widen_bytes += (width_bytes(st.dist[su].width) - u_before) + (width_bytes(st.dist[sv].width) - v_before);
#endif
      if(widened_out) *widened_out=true;
      cand128 = (unsigned __int128)base_u + w;
#else
  // Widening disabled: signal overflow as an error
  throw std::overflow_error("Distance overflow and widening disabled");
#endif
    } else {
      cand128 = cand64;
    }
  } else if(du.width==DistWidth::W128){
    unsigned __int128 base128 = du.small.v128;
    unsigned __int128 addw = (unsigned __int128)w;
    if(base128 > (std::numeric_limits<unsigned __int128>::max() - addw)){
#ifdef ENABLE_BIGINT_FALLBACK
      candBig.assign_u128(base128); candBig.add_u64_inplace(w); kind = CandKind::KBIG;
#else
      throw std::overflow_error("Distance overflow beyond 128-bit and BigInt fallback disabled");
#endif
    } else {
      cand128 = base128 + addw;
    }
  } else { // BIG
#ifdef ENABLE_BIGINT_FALLBACK
    candBig = du.big; candBig.add_u64_inplace(w); kind = CandKind::KBIG;
#else
    throw std::overflow_error("BIGINT arithmetic requested but fallback disabled");
#endif
  }

  // Early accept if dv was INF
  if(dv_was_inf){
    int new_hop = (st.hop[su] < std::numeric_limits<int>::max()) ? st.hop[su] + 1 : st.hop[su];
    if(kind==CandKind::KBIG){
#ifdef ENABLE_BIGINT_FALLBACK
      dv.big = std::move(candBig); dv.width=DistWidth::BIG; st.used_bigint=true;
#ifdef ENABLE_BMSSP_VERIFIER
      st.bigint_total_limbs += dv.big.limbs.size();
      st.big_bytes += dv.big.limbs.size()*8ULL;
#endif
#else
      return false;
#endif
    } else {
      if(cand128 <= std::numeric_limits<uint32_t>::max()){
        dv.width=DistWidth::W32; dv.small.v32 = (uint32_t)cand128;
      } else if(cand128 <= std::numeric_limits<uint64_t>::max()){
        dv.width=DistWidth::W64; dv.small.v64 = (uint64_t)cand128;
      } else {
        dv.width=DistWidth::W128; dv.small.v128 = cand128;
      }
    }
    st.pred[sv]=u; st.hop[sv]=new_hop;
#ifdef ENABLE_BMSSP_VERIFIER
    st.relax_improve_events++;
#endif
    return true;
  }

  // Compare and possibly accept
  bool better=false, equal_dist=false;
  if(kind==CandKind::KBIG){
#ifdef ENABLE_BIGINT_FALLBACK
    int c = (dv.width==DistWidth::BIG) ? BigInt::cmp(candBig, dv.big)
                                       : BigInt::cmp(candBig, distword_to_big(dv));
    better = (c < 0); equal_dist = (c==0);
    if(!(better || (allow_equal && equal_dist))) return false;
    dv.big = std::move(candBig); dv.width=DistWidth::BIG; st.used_bigint=true;
#ifdef ENABLE_BMSSP_VERIFIER
  st.bigint_total_limbs += dv.big.limbs.size();
  st.big_bytes += dv.big.limbs.size()*8ULL;
#endif
    int new_hop = (st.hop[su] < std::numeric_limits<int>::max()) ? st.hop[su] + 1 : st.hop[su];
    bool take=true; int old_hop = st.hop[sv];
    if(!better && equal_dist && allow_equal){ if(new_hop > old_hop) take=false; else if(new_hop==old_hop){ int old_pred = st.pred[sv]; if(!(old_pred<0 || u < old_pred)) take=false; } }
    if(!take) return false;
    st.pred[sv]=u; st.hop[sv]=new_hop;
#ifdef ENABLE_BMSSP_VERIFIER
    if(better) st.relax_improve_events++; else st.relax_equal_events++;
#endif
    return true;
#else
    return false;
#endif
  } else {
    // 128-bit candidate vs dv (which may be BIG)
    if(dv.width==DistWidth::BIG){
#ifdef ENABLE_BIGINT_FALLBACK
      BigInt tmp; tmp.assign_u128(cand128);
      int c = BigInt::cmp(tmp, dv.big);
      better = (c < 0); equal_dist = (c==0);
#else
      return false;
#endif
    } else {
      unsigned __int128 dv128=0;
      if(dv.width==DistWidth::W32) dv128 = dv.small.v32;
      else if(dv.width==DistWidth::W64) dv128 = dv.small.v64;
      else dv128 = dv.small.v128;
      better = (cand128 < dv128); equal_dist = (cand128 == dv128);
    }
    if(!(better || (allow_equal && equal_dist))){
#ifdef ENABLE_BMSSP_VERIFIER
      st.relax_reject_events++;
#endif
      return false;
    }
    int new_hop = (st.hop[su] < std::numeric_limits<int>::max()) ? st.hop[su] + 1 : st.hop[su];
    bool take = better; int old_hop = st.hop[sv];
    if(!take && equal_dist && allow_equal){
      if(new_hop < old_hop) take = true; else if(new_hop==old_hop){ int old_pred = st.pred[sv]; if(old_pred<0 || u < old_pred) take=true; }
    }
    if(!take){
#ifdef ENABLE_BMSSP_VERIFIER
      if(equal_dist) st.relax_reject_events++;
#endif
      return false;
    }
    if(cand128 <= std::numeric_limits<uint32_t>::max()){
      dv.width=DistWidth::W32; dv.small.v32 = (uint32_t)cand128;
    } else if(cand128 <= std::numeric_limits<uint64_t>::max()){
      dv.width=DistWidth::W64; dv.small.v64 = (uint64_t)cand128;
    } else {
      dv.width=DistWidth::W128; dv.small.v128 = cand128;
    }
    st.pred[sv]=u; st.hop[sv]=new_hop;
#ifdef ENABLE_BMSSP_VERIFIER
    if(better) st.relax_improve_events++; else st.relax_equal_events++;
#endif
    return true;
  }
}

} // namespace bmssp
