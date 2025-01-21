#ifndef LIBTEDDY_IMPL_CONFIG_HPP
#define LIBTEDDY_IMPL_CONFIG_HPP

/*
 * Use TEDDY_DEF for non-template definitions in .hpp and .cpp files
 * (which are inline included in header-only mode)
 */

/*
 * Use TEDDY_DEF_INL for non-template definitions in .hpp and .inl files
 * (which are always inline included to give compiler a chance
 *  to inline definitions)
 */

#ifdef TEDDY_NO_HEADER_ONLY
  #define TEDDY_DEF
  #define TEDDY_DEF_INL inline
#else
  #define TEDDY_DEF inline
  #define TEDDY_DEF_INL inline
#endif

#endif