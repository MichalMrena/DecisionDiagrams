#ifndef LIBTEDDY_IMPL_CONFIG_HPP
#define LIBTEDDY_IMPL_CONFIG_HPP

// Defines TEDDY_DEF_INLINE as inline in header-only mode and as empty otherwise
#ifdef TEDDY_NO_HEADER_ONLY
  #define TEDDY_DEF_INLINE
#else
  #define TEDDY_DEF_INLINE inline
#endif

#endif