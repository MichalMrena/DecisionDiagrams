#include "other_tests.hpp"

#include "../bdd/pla_file.hpp"
#include "../bdd/bdd_pla.hpp"
#include "../utils/io.hpp"

namespace mix::dd
{
    using creator_t = bdds_from_pla<empty, empty>;
    using bdd_t     = bdd<empty, empty>;

    auto test_constructors (const pla_file& file) -> bool
    {
        creator_t creator;
        const auto diagrams {creator.create(file)};

        // copy constructor
        bdd_t d1 {diagrams.at(0)};
        if (d1 != diagrams.at(0))
        {
            utils::printl("Copy constructed diagram is not equal.");
        }

        utils::printl("Constructors are correct.");
        return true;
    }
}