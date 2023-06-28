#ifndef LIBTEDDY_TSL_SYSTEM_DESCRIPTION_HPP
#define LIBTEDDY_TSL_SYSTEM_DESCRIPTION_HPP

#include <libtsl/types.hpp>

#include <vector>

namespace teddy::tsl
{
    struct system_description
    {
        int32 stateCount_;
        std::vector<int32> structureFunction_;
        std::vector<int32> domains_;
        std::vector<std::vector<double>> componentProbabilities_;
        std::vector<double> stateProbabilites_;
        std::vector<double> availabilities_;
        std::vector<double> unavailabilities_;
        std::vector<std::vector<int32>> mcvs_;

        // component index; system state; component state
        std::vector<std::vector<std::vector<double>>> structuralImportances_;
        std::vector<std::vector<std::vector<double>>> birnbaumImportances_;
        std::vector<std::vector<std::vector<double>>> fusselVeselyImportances_;
    };
}

#endif