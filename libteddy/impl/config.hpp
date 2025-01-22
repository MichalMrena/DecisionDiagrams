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

/*
 * Use TEDDY_ANON_NAMESPACE_BEGIN/END to define anonymous namespace.
 * It has no point in header-only mode.
 */

#ifdef TEDDY_NO_HEADER_ONLY

  #define TEDDY_DEF

  #define TEDDY_DEF_INL inline

  #define TEDDY_ANON_NAMESPACE_BEGIN namespace {

  #define TEDDY_ANON_NAMESPACE_END }

#else

  #define TEDDY_DEF inline

  #define TEDDY_DEF_INL inline

  #define TEDDY_ANON_NAMESPACE_BEGIN

  #define TEDDY_ANON_NAMESPACE_END

#endif

#endif