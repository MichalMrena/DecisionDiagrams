#include <libteddy/reliability.hpp>
#include <array>
#include <iostream>
#include <vector>

int main()
{
    // First, we need to create a diagram for the structure function.
    // We can use the truth vector of the function.
    std::vector<int> vector(
        {0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,2,2,2,2,2,1,2,2,2,2,2}
    );

    // The truth vector describes a nonhomogeneous system, so we also need.
    // The number of states of each component.
    std::vector<int> domains({2, 3, 2, 3});

    // 4 components, 1000 pre-allocated nodes (see memory management)
    teddy::ifmss_manager<3> manager(4, 1'000, domains);

    // Alias for the type of the diagram, or you can just use auto.
    using diagram_t = teddy::ifmss_manager<3>::diagram_t;
    diagram_t sf = manager.from_vector(vector);

    // We can use different combinations of std::vector, std::array or similar containers.
    // We chose vector of arrays here to hold component state probabilities.
    std::vector<std::array<double, 3>> ps
    ({
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    });

    // To calculate system availability or unavailability for a given.
    // System state (1) is as simple as:
    const double A = manager.calculate_availability(1, ps, sf);
    const double U = manager.calculate_unavailability(1, ps, sf);
    std::cout << "A = " << A << "\n";
    std::cout << "U = " << U << "\n";

    // We can also simply enumerate all Minimal Cut Vectors for a given system
    // state (1). We just need to specify a type that will store the vectors.
    // In this case, we used std::array:
    std::vector<std::array<int, 4>> MCVs = manager.mcvs<std::array<int, 4>>(sf, 1);

    // Importance measures are defined in terms of logic derivatives.
    // Since there are different types of derivatives the calculation of
    // the derivatives is separated from the calculation of importance measures.

    // To calculate Structural Importance we first need to calculate
    // the derivative.
    diagram_t dpbd = manager.dpld({2, 1, 0}, teddy::dpld::type_3_decrease(1), sf);

    // Now, to calculate the Structural Importance of the second component,
    // we use the derivative.
    const double SI_2 = manager.structural_importance(dpbd);
    std::cout << "SI_2 = " << SI_2 << "\n";
}