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
 * Adapted to a jemalloc-backed global allocator override and transformed by
 * large language models, as requested by the user.
 */
// Global new/delete overrides using jemalloc when available.
// Enabled via ENABLE_GLOBAL_JEMALLOC in CMake.

#include <new>
#include <cstddef>

#ifdef HAVE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

// Sized delete/new support is automatic with Clang if sized deallocation is enabled.
// We provide the common overloads.

void* operator new(std::size_t sz) {
#ifdef HAVE_JEMALLOC
  if (void* p = mallocx(sz, 0)) return p;
  throw std::bad_alloc();
#else
  return ::operator new(sz);
#endif
}

void* operator new(std::size_t sz, const std::nothrow_t&) noexcept {
#ifdef HAVE_JEMALLOC
  return mallocx(sz, 0);
#else
  try {
    return ::operator new(sz);
  } catch (...) {
    return nullptr;
  }
#endif
}

void* operator new[](std::size_t sz) {
#ifdef HAVE_JEMALLOC
  if (void* p = mallocx(sz, 0)) return p;
  throw std::bad_alloc();
#else
  return ::operator new[](sz);
#endif
}

void* operator new[](std::size_t sz, const std::nothrow_t&) noexcept {
#ifdef HAVE_JEMALLOC
  return mallocx(sz, 0);
#else
  try {
    return ::operator new[](sz);
  } catch (...) {
    return nullptr;
  }
#endif
}

void operator delete(void* p) noexcept {
#ifdef HAVE_JEMALLOC
  if (p) dallocx(p, 0);
#else
  ::operator delete(p);
#endif
}

void operator delete(void* p, std::size_t) noexcept {
#ifdef HAVE_JEMALLOC
  if (p) dallocx(p, 0);
#else
  ::operator delete(p);
#endif
}

void operator delete[](void* p) noexcept {
#ifdef HAVE_JEMALLOC
  if (p) dallocx(p, 0);
#else
  ::operator delete[](p);
#endif
}

void operator delete[](void* p, std::size_t) noexcept {
#ifdef HAVE_JEMALLOC
  if (p) dallocx(p, 0);
#else
  ::operator delete[](p);
#endif
}
