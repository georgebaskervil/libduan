// License: Placeholder - add your license details here.
// Optional: on startup, print jemalloc version if available.

#include <iostream>

#ifdef HAVE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

namespace {
struct JemallocDiag {
  JemallocDiag() {
#ifdef HAVE_JEMALLOC
    const char* ver = nullptr;
    if (mallctl("version", &ver, nullptr, nullptr, 0) == 0 && ver) {
      std::clog << "[jemalloc] version: " << ver << "\n";
    } else {
      std::clog << "[jemalloc] active (version unavailable)\n";
    }
#else
    (void)sizeof(0);
#endif
  }
};

static JemallocDiag g_jemalloc_diag;
}  // namespace
