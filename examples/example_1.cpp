#include <libteddy/reliability.hpp>
#include <algorithm>
#include <iostream>
#include <random>
#include <string_view>

// TODO description and ASCII RBD

int main()
{
    auto const perStringTurbineCount = 3;
    auto const stringCount = 4;
    auto const varCount = perStringTurbineCount * stringCount;

    // auto rngp = std::mt19937_64(123123);
    // auto distp = std::uniform_real_distribution(.0, 1.);
    // auto ps = std::vector<std::vector<double>>(varCount);
    // std::ranges::generate_n(begin(ps), varCount, [&]()
    // {
    //     auto const p = distp(rngp);
    //     return std::vector<double> {1 - p, p};
    // });
    auto const ps = std::vector<std::array<double, 2>>
    ({
        {0.1, 0.9},
        {0.2, 0.8},
        {0.2, 0.8},
        {0.1, 0.9},
        {0.2, 0.8},
        {0.2, 0.8},
        {0.1, 0.9},
        {0.3, 0.7},
        {0.1, 0.9},
        {0.2, 0.8},
        {0.1, 0.9},
        {0.3, 0.7},
    });
    // auto ps = std::vector<std::vector<double>>(varCount);
    // std::ranges::generate_n(begin(ps), varCount, []()
    // {
    //     return std::vector<double> {0.5, 0.5};
    // });

    using namespace teddy::ops;
    using manager_t = teddy::bss_manager;
    auto manager = manager_t(varCount, 100'000);
    auto sf = manager.constant(0);
    auto nextVar = 0;

    for (auto i = 0; i < stringCount; ++i)
    {
        auto series = manager.variable(nextVar);
        ++nextVar;
        for (auto j = 1; j < perStringTurbineCount; ++j)
        {
            series = manager.apply<AND>(
                series,
                manager.variable(nextVar)
            );
            ++nextVar;
        }
        sf = manager.apply<OR>(sf, series);
    }

    std::cout << "A = "   << manager.availability(ps, sf)   << "\n";
    std::cout << "U = "   << manager.unavailability(ps, sf) << "\n";
    std::cout << "Fr1 = " << manager.state_frequency(sf, 1) << "\n";
    std::cout << "Fr0 = " << manager.state_frequency(sf, 0) << "\n";
}