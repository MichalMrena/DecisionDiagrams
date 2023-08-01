#ifndef LIBTEDDY_DETAILS_CONFIG_HPP
#define LIBTEDDY_DETAILS_CONFIG_HPP

/**
 *  Enables verbose output anouncing:
 *    - garbage collection,
 *    - node pool allocations,
 *    - unique table rehashing,
 *    - apply cache rehashing,
 *    - and possibly other events.
 */
// #define LIBTEDDY_VERBOSE

/**
 *  Enables collection of statistics.
 *  They can be viewed using teddy::dump_stats().
 */
#define LIBTEDDY_COLLECT_STATS

#endif