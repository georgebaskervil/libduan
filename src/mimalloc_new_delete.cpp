// Override global new/delete to use mimalloc
// This ensures all C++ allocations go through mimalloc

#ifdef HAVE_MIMALLOC

#include <mimalloc.h>
#include <new>

// Override new
void* operator new(std::size_t size) {
  void* p = mi_malloc(size);
  if (!p) throw std::bad_alloc();
  return p;
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  return mi_malloc(size);
}

// Override new[]
void* operator new[](std::size_t size) {
  void* p = mi_malloc(size);
  if (!p) throw std::bad_alloc();
  return p;
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
  return mi_malloc(size);
}

// Override delete
void operator delete(void* p) noexcept {
  mi_free(p);
}

void operator delete(void* p, const std::nothrow_t&) noexcept {
  mi_free(p);
}

void operator delete(void* p, std::size_t) noexcept {
  mi_free(p);
}

// Override delete[]
void operator delete[](void* p) noexcept {
  mi_free(p);
}

void operator delete[](void* p, const std::nothrow_t&) noexcept {
  mi_free(p);
}

void operator delete[](void* p, std::size_t) noexcept {
  mi_free(p);
}

// C++17 aligned allocation
void* operator new(std::size_t size, std::align_val_t alignment) {
  void* p = mi_malloc_aligned(size, static_cast<std::size_t>(alignment));
  if (!p) throw std::bad_alloc();
  return p;
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
  return mi_malloc_aligned(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
  void* p = mi_malloc_aligned(size, static_cast<std::size_t>(alignment));
  if (!p) throw std::bad_alloc();
  return p;
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
  return mi_malloc_aligned(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* p, std::align_val_t) noexcept {
  mi_free(p);
}

void operator delete(void* p, std::size_t, std::align_val_t) noexcept {
  mi_free(p);
}

void operator delete[](void* p, std::align_val_t) noexcept {
  mi_free(p);
}

void operator delete[](void* p, std::size_t, std::align_val_t) noexcept {
  mi_free(p);
}

#endif // HAVE_MIMALLOC
