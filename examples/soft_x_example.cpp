#include <libteddy/reliability.hpp>
#include <array>
#include <iostream>
#include <vector>
int main() {
    using namespace teddy::dpld;
    using namespace teddy::ops;
    using mdd_t = teddy::mss_manager<3>::diagram_t;
    teddy::mss_manager<3> manager(3, 100);
    mdd_t x0 = manager.variable(0);
    mdd_t x1 = manager.variable(1);
    mdd_t x2 = manager.variable(2);
    mdd_t g  = manager.apply<MIN>(x1, x2);
    mdd_t sf = manager.apply<MAX>(x0, g);
    std::array ps {
        std::array{.1, .7, .2},
        std::array{.3, .6, .1},
        std::array{.2, .7, .1}
    };
    mdd_t dpldX1         = manager.dpld({0,0,1}, type_3_increase(1), sf);
    int state            = manager.evaluate(sf, std::array{0,1,2});
    double availability1 = manager.calculate_availability(1, ps, sf);
    double biX1          = manager.birnbaum_importance(ps, dpldX1);
    double satCount      = manager.state_frequency(sf, 1);
    std::vector mcvs     = manager.mcvs<std::array<int, 3>>(sf, 1);

    std::cout << "state = " << state << "\n";
    std::cout << "A1    = " << availability1 << "\n";
    std::cout << "BI_x1 = " << biX1 << "\n";
    std::cout << "SF1   = " << satCount << "\n";
    std::cout << "MCVs  = ";
    for (auto const& v : mcvs)
    {
        std::cout << v[0] << v[1] << v[2] << " ";
    }
    std::cout << "\n";
}