#include "vector_function.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <libteddy/teddy_reliability.hpp>
#include <random>
#include <tuple>
#include <vector>

namespace teddy::test
{
using probability_t = double;

struct system_characteristics
{
    std::vector<probability_t> Ps_;
    std::vector<probability_t> As_;
    std::vector<probability_t> Us_;
    std::vector<std::vector<std::vector<probability_t>>> SIs_;
    std::vector<std::vector<std::vector<uint_t>>> MCVs_;
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

auto constexpr char_ok() { return "✓"; }

auto constexpr char_err() { return "!"; }

auto result_char(bool const b)
{
    return b ? wrap_green(char_ok()) : wrap_red(char_err());
}

template<uint_t P, class Manager, class Diagram, class Probabilities>
auto analyze_system(Manager& manager, Diagram& sf, Probabilities const& ps)
{
    return system_characteristics {
        .Ps_ = utils::fill_vector(
            P,
            [&](auto const j)
            {
                return manager.probability(j, ps, sf);
            }
        ),
        .As_ = utils::fill_vector(
            P,
            [&](auto const j)
            {
                return manager.availability(j, ps, sf);
            }
        ),
        .Us_ = utils::fill_vector(
            P,
            [&](auto const j)
            {
                return manager.unavailability(j, ps, sf);
            }
        ),
        .SIs_ = utils::fill_vector(
            manager.get_var_count(),
            [&](auto const i)
            {
                return utils::fill_vector(
                    P - 1,
                    [&](auto const j)
                    {
                        return utils::fill_vector(
                            manager.get_domains()[i] - 1,
                            [&](auto const v)
                            {
                                auto dpbd = manager.idpbd_type_3_decrease(
                                    {v + 1, v}, j + 1, sf, i
                                );
                                return manager.structural_importance(dpbd);
                            }
                        );
                    }
                );
            }
        ),
        .MCVs_ = utils::fill_vector(
            P - 1,
            [&](auto const j)
            {
                return manager.template mcvs<std::vector<uint_t>>(sf, j + 1);
            }
        )};
}

template<class Probabilities>
auto analyze_system(vector::vector_function const& sf, Probabilities const& ps)
{
    auto rel = teddy::vector::vector_reliability(sf, ps);
    return system_characteristics {
        .Ps_ = utils::fill_vector(
            sf.max_value() + 1,
            [&](auto const j)
            {
                return rel.probability(j);
            }
        ),
        .As_ = utils::fill_vector(
            sf.max_value() + 1,
            [&](auto const j)
            {
                return rel.availability(j);
            }
        ),
        .Us_ = utils::fill_vector(
            sf.max_value() + 1,
            [&](auto const j)
            {
                return rel.unavailability(j);
            }
        ),
        .SIs_ = utils::fill_vector(
            sf.get_var_count(),
            [&](auto const i)
            {
                return utils::fill_vector(
                    sf.max_value(),
                    [&](auto const j)
                    {
                        return utils::fill_vector(
                            sf.get_domains()[i] - 1,
                            [&](auto const v)
                            {
                                return rel.structural_importance(
                                    j + 1, {i, v + 1, v}
                                );
                            }
                        );
                    }
                );
            }
        ),
        .MCVs_ = utils::fill_vector(
            sf.max_value(),
            [&](auto const j)
            {
                return rel.mcvs(j + 1);
            }
        )};
}

auto evaluate_test(
    system_characteristics const& expected, system_characteristics const& actual
)
{
    namespace rs   = std::ranges;
    auto const cmp = [](auto const l, auto const r)
    {
        return std::abs(l - r) < 0.000001;
    };

    return std::vector<bool>({// Probabilities.
                              std::ranges::equal(expected.Ps_, actual.Ps_, cmp),

                              // Availabilities.
                              std::ranges::equal(expected.As_, actual.As_, cmp),

                              // Unavailabilities.
                              std::ranges::equal(expected.Us_, actual.Us_, cmp),

                              // Structural importances.
                              std::ranges::equal(
                                  expected.SIs_, actual.SIs_,
                                  [cmp](auto const& lhs, auto const& rhs)
                                  {
                                      return rs::equal(
                                          lhs, rhs,
                                          [cmp](auto const& l, auto const& r)
                                          {
                                              return rs::equal(l, r, cmp);
                                          }
                                      );
                                  }
                              ),

                              // Minimal Cut Vectors.
                              [&]()
                              {
                                  for (auto j = 0u; j < expected.MCVs_.size();
                                       ++j)
                                  {
                                      if (not std::ranges::is_permutation(
                                              expected.MCVs_[j], actual.MCVs_[j]
                                          ))
                                      {
                                          return false;
                                      }
                                  }
                                  return true;
                              }()});
}

auto print_test_evaluation(
    system_characteristics const& expected, system_characteristics const& actual
)
{
    namespace rs   = std::ranges;
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
    std::cout << "SIs              "
              << result_char(rs::equal(
                     expected.SIs_, actual.SIs_,
                     [cmp](auto const& lhs, auto const& rhs)
                     {
                         return rs::equal(
                             lhs, rhs,
                             [cmp](auto const& l, auto const& r)
                             {
                                 return rs::equal(l, r, cmp);
                             }
                         );
                     }
                 ))
              << '\n';
    std::cout << "MCVs             "
              << result_char(
                     [&]()
                     {
                         for (auto j = 0u; j < expected.MCVs_.size(); ++j)
                         {
                             if (not rs::is_permutation(
                                     expected.MCVs_[j], actual.MCVs_[j]
                                 ))
                             {
                                 return false;
                             }
                         }
                         return true;
                     }()
                 )
              << '\n';
}

template<uint_t P>
auto generate_serialparallel(
    mss_manager<P>& manager, std::mt19937_64& rngtype,
    std::mt19937_64& rngbranch
)
{
    using namespace teddy::ops;

    auto go = [&, i = 0u](auto& self, auto const n) mutable
    {
        if (n == 1)
        {
            return manager.variable(i++);
        }
        else
        {
            auto denomdist     = std::uniform_int_distribution<uint_t>(2, 10);
            auto typedist      = std::uniform_real_distribution(0.0, 1.0);
            auto const denom   = denomdist(rngbranch);
            auto const lhssize = std::max(1u, n / denom);
            auto const rhssize = n - lhssize;
            auto const lhs     = self(self, lhssize);
            auto const rhs     = self(self, rhssize);
            auto const p       = typedist(rngtype);
            return p < 0.5 ? manager.template apply<MIN>(lhs, rhs)
                           : manager.template apply<MAX>(lhs, rhs);
        }
    };
    return go(go, static_cast<uint_t>(manager.get_var_count()));
}

template<uint_t P>
auto generate_probabilities(std::size_t const n, std::mt19937_64& rngp)
{
    auto pdist = std::uniform_real_distribution<double>();
    auto ps    = utils::fill_vector(
        n,
        [&](auto)
        {
            return utils::fill_vector(
                P,
                [&](auto const)
                {
                    return pdist(rngp);
                }
            );
        }
    );
    for (auto& varps : ps)
    {
        auto const sum = std::reduce(begin(varps), end(varps), 0.0);
        for (auto& p : varps)
        {
            p /= sum;
        }
    }
    return ps;
}

template<uint_t P>
auto test_n_random(std::size_t const testCount, std::size_t const n)
{
    auto seeder    = std::mt19937_64(144);
    auto rngtype   = std::mt19937_64(seeder());
    auto rngbranch = std::mt19937_64(seeder());
    auto rngp      = std::mt19937_64(seeder());
    auto results   = std::vector<std::vector<bool>>();

    for (auto k = 0u; k < testCount; ++k)
    {
        auto manager    = mss_manager<P>(n, 10'000);
        auto diagram    = generate_serialparallel(manager, rngtype, rngbranch);
        auto vectorFunc = vector::vector_function(
            manager.to_vector(diagram),
            utils::fill_vector(n, utils::constant(P))
        );
        auto const ps       = generate_probabilities<P>(n, rngp);
        auto const actual   = analyze_system<P>(manager, diagram, ps);
        auto const expected = analyze_system(vectorFunc, ps);
        results.emplace_back(evaluate_test(actual, expected));
    }

    auto const print_row = [](auto const& name, auto const& rs, auto const row)
    {
        std::cout << name;
        for (auto col = 0u; col < rs.size(); ++col)
        {
            std::cout << result_char(rs[col][row]) << " ";
        }
        std::cout << '\n';
    };

    print_row("probabilities    ", results, 0u);
    print_row("availabilities   ", results, 1u);
    print_row("unavailabilities ", results, 2u);
    print_row("SIs              ", results, 3u);
    print_row("MCVs             ", results, 4u);
}
} // namespace teddy::test

auto system_1()
{
    auto const vector = std::vector<teddy::uint_t> {
        0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
        0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2};
    auto domains        = std::vector<teddy::uint_t>({2, 3, 2, 3});
    auto const ps       = std::vector<std::vector<double>>({
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    });
    auto vectorSf       = teddy::vector::vector_function(vector, domains);
    auto manager        = teddy::ifmss_manager<3>(4, 1'000, {2, 3, 2, 3});
    auto diagram        = manager.from_vector(vector);
    auto const actual   = teddy::test::analyze_system<3>(manager, diagram, ps);
    auto const expected = teddy::test::analyze_system(vectorSf, ps);
    teddy::test::print_test_evaluation(expected, actual);
}

int main()
{
    std::cout << "Fixed system:" << '\n';
    system_1();
    std::cout << '\n';

    std::cout << "Random systems:" << '\n';
    teddy::test::test_n_random<3>(50, 10);
}