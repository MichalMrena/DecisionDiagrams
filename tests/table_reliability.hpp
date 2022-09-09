#ifndef LIBTEDDY_TESTS_TABLE_RELIABILITY_HPP
#define LIBTEDDY_TESTS_TABLE_RELIABILITY_HPP

#include "truth_table.hpp"

#include <vector>

namespace teddy
{
    auto probability(
        truth_table const& table, std::vector<std::vector<double>> const& ps,
        unsigned int j
    ) -> double;

    auto availability(
        truth_table const& table, std::vector<std::vector<double>> const& ps,
        unsigned int j
    ) -> double;

    auto unavailability(
        truth_table const& table, std::vector<std::vector<double>> const& ps,
        unsigned int j
    ) -> double;
}

#endif