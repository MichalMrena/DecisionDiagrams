#include "../include/teddy/teddy_reliability.hpp"
#include <array>
#include <cmath>
#include <vector>

namespace teddy::test
{
    using probability_t = double;

    struct system_characteristics
    {
        std::vector<probability_t> Ps_;
        std::vector<probability_t> As_;
        std::vector<probability_t> Us_;
    };

    auto wrap_green(std::string_view const s)
    {
        return std::string("\x1B[92m") + std::string(s) + "\x1B[0m";
    }

    auto wrap_red(std::string_view const s)
    {
        return std::string("\x1B[91m") + std::string(s) + "\x1B[0m";
    }

    auto wrap_yellow(std::string_view const s)
    {
        return std::string("\x1B[93m") + std::string(s) + "\x1B[0m";
    }

    auto constexpr char_ok()
    {
        return "✓";
    }

    auto constexpr char_err()
    {
        return "!";
    }

    auto result_char (bool const b)
    {
        return b ? wrap_green(char_ok()) : wrap_red(char_err());
    }

    template<uint_t P, class Manager, class Diagram, class Probabilities>
    auto analyze_system (Manager& manager, Diagram& sf, Probabilities ps)
    {
        return system_characteristics
        {
            .Ps_ = utils::fill_vector(P, [&](auto const j)
            {
                return manager.probability(j, ps, sf);
            }),
            .As_ = utils::fill_vector(P, [&](auto const j)
            {
                return manager.availability(j, ps, sf);
            }),
            .Us_ = utils::fill_vector(P, [&](auto const j)
            {
                return manager.unavailability(j, ps, sf);
            })
        };
    }

    auto evaluate_test ( system_characteristics expected
                       , system_characteristics actual )
    {
        namespace rs = std::ranges;
        auto const cmp = [](auto const l, auto const r)
        {
            return std::abs(l - r) < 0.000001;
        };

        std::cout << "probabilities    "
            << result_char(rs::equal(expected.Ps_, actual.Ps_, cmp)) << '\n';
        std::cout << "availabilities   "
            << result_char(rs::equal(expected.As_, actual.As_, cmp)) << '\n';
        std::cout << "unavailabilities "
            << result_char(rs::equal(expected.Us_, actual.Us_, cmp)) << '\n';
    }

}

auto system_1()
{
    namespace ts = teddy::test;
    auto const expected = ts::system_characteristics
    {
        .Ps_ = {0.0084, 0.2932, 0.6984},
        .As_ = {1.0000, 0.9916, 0.6984},
        .Us_ = {0.0000, 0.0084, 0.3016}
    };
    auto manager = teddy::ifmss_manager<3>(4, 1'000, {2, 3, 2, 3});
    auto const vector = std::vector<teddy::uint_t>
        { 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1
        , 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2 };
    auto diagram = manager.from_vector(vector);
    auto const ps = std::vector<std::array<double, 3>>
    {{
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    }};
    auto const actual = ts::analyze_system<3>(manager, diagram, ps);
    ts::evaluate_test(expected, actual);
}

int main()
{
    // TODO vyskúšať posielať diagramy všade hodnotami,
    // mohlo by to byť krajšie a na funkciu by to nemalo mať žiadny vplyv
    system_1();
}