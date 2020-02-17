#ifndef _MIX_DD_PLA_HEURISTIC_
#define _MIX_DD_PLA_HEURISTIC_

namespace mix::dd
{
    class pla_file;

    auto improve_ordering (pla_file& file) -> pla_file&;
    // auto improve_ordering_p (pla_file& file) -> pla_file&;
}

#endif