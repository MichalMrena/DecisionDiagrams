#ifndef LIBTEDDY_DETAILS_CONFIG_HPP
#define LIBTEDDY_DETAILS_CONFIG_HPP

/**
 *  Enables verbose output anouncing:
 *    - garbage collection,
 *    - node pool allocations,
 *    - unique table rehashing,
 *    - apply cache rehashing,
 *    - and possibly other events.
 *
 *  This option can also be enabled in the root CMakeLists.txt
 */
// #define LIBTEDDY_VERBOSE

/**
 *  Enables collection of statistics.
 *  They can be viewed using teddy::dump_stats().
 *
 *  This option can also be enabled in the root CMakeLists.txt
 */
// #define LIBTEDDY_COLLECT_STATS

/**
 *  Enables symbolic probabilistic evaluation
 *  See the documentation for dependencies
 *
 *  This option is here just for the sake of completness, it should
 *  only be set in the root CMakeLists.txt since it adds
 *  some additional linking flags
 */
// #define LIBTEDDY_SYMBOLIC_RELIABILITY

#endif