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
            utils::printl("!!! Copy constructed diagram is not equal.");
            return false;
        }

        // copy assign
        bdd_t d2 {bdd_t::just_false()};
        d2 = d1;
        if (d1 != d2)
        {
            utils::printl("!!! Copy assigned diagram is not equal.");
            return false;
        }

        // move constructor
        bdd_t d3 {std::move(d2)};
        if (d1 != d3)
        {
            utils::printl("!!! Move constructed diagram is not equal.");
            return false;
        }

        if (d2 == d3)
        {
            utils::printl("!!! Moved from diagram is equal to move constructed diagram.");
            return false;
        }

        // empty copy
        bdd_t e1 {bdd_t::just_false()};
        bdd_t e2 {e1};
        if (e1 != e2)
        {
            utils::printl("!!! Empty copies are not equal.");
            return false;
        }

        utils::printl("Constructors are correct.");
        return true;
    }
}