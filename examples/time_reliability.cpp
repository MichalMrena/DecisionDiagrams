#include <libteddy/reliability.hpp>

#include <iostream>
#include <vector>

int main()
{
    namespace tps = teddy::probs;
    using namespace teddy::ops;
    using bdd = teddy::bss_manager::diagram_t;
    teddy::bss_manager manager(3, 1'000);
    auto& x = manager;
    bdd f = manager.apply<AND>(x(0), manager.apply<OR>(x(1), x(2)));
    manager.to_dot_graph(std::cout, f);
    std::vector<tps::prob_dist> ps(
    {
        {tps::weibull(1, 1)},
        {tps::exponential(0.5)},
        {tps::exponential(0.5)}
    });

    for (double t = 0.1; t < 10; t += 1)
    {
        double const A = manager.calculate_availability(tps::at_time(ps, t), f);
        std::cout << "t = " << t << " A = " << A << "\n";
    }
}