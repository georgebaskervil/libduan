/* The following code example is taken from the book
 * "The C++ Standard Library - A Tutorial and Reference"
 * by Nicolai M. Josuttis, Addison-Wesley, 1999
 *
 * (C) Copyright Nicolai M. Josuttis 1999.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 *
 * Adapted to a jemalloc-backed allocator and transformed by large language models,
 * as requested by the user.
 */
// A minimal C++ allocator that uses jemalloc when available.

#pragma once

#include <cstddef>
#include <limits>
#include <new>

#ifdef HAVE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

namespace cpp_starter {

template <class T>
struct JemallocAllocator {
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  JemallocAllocator() noexcept = default;
  template <class U>
  JemallocAllocator(const JemallocAllocator<U>&) noexcept {}

  [[nodiscard]] T* allocate(std::size_t n) {
    if (n > max_size()) throw std::bad_alloc();
#ifdef HAVE_JEMALLOC
    int flags = 0;
#if defined(__cpp_aligned_new) || (defined(__GNUC__) || defined(__clang__))
    if constexpr (alignof(T) > alignof(std::max_align_t)) {
      // MALLOCX_ALIGN expects a power-of-two alignment.
      flags |= MALLOCX_ALIGN(alignof(T));
    }
#endif
    void* p = mallocx(n * sizeof(T), flags);
#else
  void* p = ::operator new(n * sizeof(T));
#endif
    if (!p) throw std::bad_alloc();
    return static_cast<T*>(p);
  }

  void deallocate(T* p, std::size_t /*n*/) noexcept {
#ifdef HAVE_JEMALLOC
  dallocx(static_cast<void*>(p), 0);
#else
  ::operator delete(static_cast<void*>(p));
#endif
  }

  constexpr std::size_t max_size() const noexcept {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  template <class U>
  struct rebind {
    using other = JemallocAllocator<U>;
  };
};

template <class T, class U>
inline bool operator==(const JemallocAllocator<T>&, const JemallocAllocator<U>&) noexcept {
  return true;
}

template <class T, class U>
inline bool operator!=(const JemallocAllocator<T>&, const JemallocAllocator<U>&) noexcept {
  return false;
}

}  // namespace cpp_starter
