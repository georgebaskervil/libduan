/* mimalloc_force.h - Force mimalloc malloc override */
#ifndef MIMALLOC_FORCE_H
#define MIMALLOC_FORCE_H

#ifdef HAVE_MIMALLOC
  #include <mimalloc.h>
  #include <mimalloc-override.h>
#endif

#endif /* MIMALLOC_FORCE_H */
