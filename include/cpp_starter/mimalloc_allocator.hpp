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
 * Adapted to a mimalloc-backed allocator.
 */
// A minimal C++ allocator that uses mimalloc when available.

#pragma once

#include <cstddef>
#include <limits>
#include <new>

#ifdef HAVE_MIMALLOC
#include <mimalloc.h>
#endif

namespace cpp_starter {

template <class T>
struct MimallocAllocator {
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  MimallocAllocator() noexcept = default;
  template <class U>
  MimallocAllocator(const MimallocAllocator<U>&) noexcept {}

  [[nodiscard]] T* allocate(std::size_t n) {
    if (n > max_size()) throw std::bad_alloc();
#ifdef HAVE_MIMALLOC
    // Use aligned allocation if needed
    if constexpr (alignof(T) > alignof(std::max_align_t)) {
      void* p = mi_malloc_aligned(n * sizeof(T), alignof(T));
      if (!p) throw std::bad_alloc();
      return static_cast<T*>(p);
    } else {
      void* p = mi_malloc(n * sizeof(T));
      if (!p) throw std::bad_alloc();
      return static_cast<T*>(p);
    }
#else
    return static_cast<T*>(::operator new(n * sizeof(T)));
#endif
  }

  void deallocate(T* p, std::size_t) noexcept {
#ifdef HAVE_MIMALLOC
    mi_free(p);
#else
    ::operator delete(p);
#endif
  }

  [[nodiscard]] static constexpr size_type max_size() noexcept {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }

  template <class U>
  void destroy(U* p) {
    p->~U();
  }

  template <class U>
  struct rebind {
    using other = MimallocAllocator<U>;
  };
};

template <class T, class U>
constexpr bool operator==(const MimallocAllocator<T>&, const MimallocAllocator<U>&) noexcept {
  return true;
}

template <class T, class U>
constexpr bool operator!=(const MimallocAllocator<T>&, const MimallocAllocator<U>&) noexcept {
  return false;
}

}  // namespace cpp_starter
