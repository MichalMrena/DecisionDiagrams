#ifndef LIBTEDDY_TSL_SYSTEM_DESCRIPTION_HPP
#define LIBTEDDY_TSL_SYSTEM_DESCRIPTION_HPP

#include <libtsl/types.hpp>

#include <ostream>
#include <vector>

namespace teddy::tsl
{
    struct system_description
    {
        int32 stateCount_;
        int32 componentCount_;
        std::vector<int32> structureFunction_;
        std::vector<int32> domains_;
        std::vector<std::vector<double>> componentProbabilities_;
        std::vector<double> stateProbabilities_;
        std::vector<double> availabilities_;
        std::vector<double> unavailabilities_;
        std::vector<std::vector<int32>> mcvs_;
        std::vector<std::vector<int32>> mpvs_;

        // component index; system state; component state
        std::vector<std::vector<std::vector<double>>> structuralImportances_;
        std::vector<std::vector<std::vector<double>>> birnbaumImportances_;
        std::vector<std::vector<std::vector<double>>> fusselVeselyImportances_;
    };

    inline auto operator<< (std::ostream& ost, system_description const& system) -> std::ostream&
    {
        ost << "[system]";
        return ost;
    }
}

#endif