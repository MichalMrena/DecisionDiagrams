#include <libteddy/inc/io.hpp>
#include <libteddy/inc/reliability.hpp>
#include <libtsl/generators.hpp>

#include <array>
#include <iostream>
#include <random>

auto make_basic_probs (
    int const varCount,
    std::ranlux48& rng
) -> std::vector<std::array<teddy::probs::prob_dist, 2>>
{
    using namespace teddy::probs; // NOLINT
    std::vector<std::array<prob_dist, 2>> probs;
    for (int i = 0; i < varCount; ++i)
    {
        std::uniform_real_distribution<double> distRate(0.2, 1.0);
        double const rate = distRate(rng);
        probs.push_back(std::array<prob_dist, 2>{
            complemented_exponential(rate),
            exponential(rate)
        });
    }
    return probs;
}

auto make_precise_probs (
    int const varCount,
    std::ranlux48& rng,
    mpfr::mpreal const& t
) -> std::vector<std::array<mpfr::mpreal, 2>>
{
    std::vector<std::array<mpfr::mpreal, 2>> probs;
    for (int i = 0; i < varCount; ++i)
    {
        std::uniform_real_distribution<double> distRate(0.2, 1.0);
        mpfr::mpreal const rate(distRate(rng));
        probs.push_back(std::array<mpfr::mpreal, 2>{
            mpfr::exp(-rate * t),
            1 - mpfr::exp(-rate * t)
        });
    }
    return probs;
}

auto main() -> int
{
    const int digits = 500;
    mpfr::mpreal::set_default_prec(mpfr::digits2bits(digits));

    using namespace teddy; // NOLINT
    std::optional<pla_file> fileOpt = pla_file::load_file(
        "/home/michal/data/IWLS93/pla/Adders/15-adder_col.pla"
    );
    if (not fileOpt.has_value())
    {
        return 1;
    }
    pla_file& file = *fileOpt;
    bss_manager manager(file.get_variable_count(), 100'000);
    std::vector<bdd_t> const bdds = io::from_pla(manager, file);
    bdd_t const& f = bdds.front();

    constexpr double Time = 0.5;
    std::ranlux48 rngBasic(911);    // NOLINT
    std::ranlux48 rngSymbolic(911); // NOLINT
    std::ranlux48 rngPrecise(911);  // NOLINT
    auto psBasic    = make_basic_probs(manager.get_var_count(), rngBasic);
    auto psSymbolic = symprobs::to_matrix(
        tsl::make_time_symprobability_vector(manager.get_var_count(), rngSymbolic)
    );
    auto psPrecise = make_precise_probs(manager.get_var_count(), rngPrecise, Time);

    double const ABasic = manager.calculate_availability(1, probs::eval_at(psBasic, Time), f);
    double const ASymbolic = manager.symbolic_availability(1, psSymbolic, f).evaluate(Time);
    mpfr::mpreal const APrecise = manager.precise_availability(psPrecise, f);

    std::cout << "Nodes     = " << manager.get_node_count(f) << "\n";
    std::cout.precision(digits);
    std::cout << "ABasic    = " << ABasic    << "\n";
    std::cout << "ASymbolic = " << ASymbolic << "\n";
    std::cout << "APrecise  = " << APrecise  << "\n";
}