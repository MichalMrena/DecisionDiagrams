#include <libteddy/reliability.hpp>

#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    namespace tp = teddy::probs;
    namespace tsp = teddy::symprobs;
    using namespace teddy::ops;
    using bdd = teddy::bss_manager::diagram_t;
    teddy::bss_manager manager(3, 1'000);
    auto& x = manager;
    bdd f = manager.apply<AND>(x(0), manager.apply<OR>(x(1), x(2)));
    // manager.to_dot_graph(std::cout, f);
    std::vector<tp::prob_dist> ps(
    {
        {tp::weibull(1, 1)},
        {tp::exponential(0.5)},
        {tp::exponential(0.5)}
    });

    std::vector<std::vector<tsp::expression>> eps({
        {tsp::complement(tsp::weibull(1, 1)), tsp::weibull(1, 1)},
        {tsp::complement(tsp::exponential(0.5)), tsp::exponential(0.5)},
        {tsp::complement(tsp::exponential(0.5)), tsp::exponential(0.5)}
    });

    tsp::expression aExpr = manager.symbolic_availability(1, eps, f);
    aExpr.to_matlab(std::cout);
    std::cout << "\n";
    aExpr.to_latex(std::cout);
    std::cout << "\n";

    for (double t = 0.1; t < 10; t += 1)
    {
        double const A = manager.calculate_availability(tp::eval_at(ps, t), f);
        std::cout << std::fixed << std::setprecision(7);
        std::cout << "t  = " << t                 << "\t"
                  << "A1 = " << A                 << "\t"
                  << "A2 = " << aExpr.evaluate(t) << "\n";
    }
}