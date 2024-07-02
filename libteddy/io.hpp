#ifndef LIBTEDDY_CORE_IO_HPP
#define  LIBTEDDY_CORE_IO_HPP

#include <libteddy/details/pla_file.hpp>
#include <libteddy/core.hpp>

#include <iterator>

namespace teddy
{
struct io
{
    /**
     *  \brief Creates BDDs defined by PLA file
     *
     *  \param manager Diagram manager
     *  \param file PLA file loaded in the instance of \c pla_file class
     *  \param foldType fold type used in diagram creation
     *  \return Vector of diagrams
     */
    static auto from_pla(
        bdd_manager& manager,
        pla_file const& file,
        fold_type const foldType = fold_type::Tree
    ) -> std::vector<bdd_manager::diagram_t>;

    /**
     *  \brief Creates diagram from a truth vector of a function
     *
     *  Example for the function f(x) = max(x0, x1, x2):
     *  \code
     *  // Truth table:
     *  +----+----+----+----++----+-----+----+---+
     *  | x1 | x2 | x3 | f  || x1 |  x2 | x3 | f |
     *  +----+----+----+----++----+-----+----+---+
     *  | 0  | 0  | 0  | 0  || 1  |  0  | 0  | 1 |
     *  | 0  | 0  | 1  | 1  || 1  |  0  | 1  | 1 |
     *  | 0  | 0  | 2  | 2  || 1  |  0  | 2  | 2 |
     *  | 0  | 1  | 0  | 1  || 1  |  1  | 0  | 1 |
     *  | 0  | 1  | 1  | 1  || 1  |  1  | 1  | 1 |
     *  | 0  | 1  | 2  | 2  || 1  |  1  | 2  | 2 |
     *  +----+----+----+----++----+-----+----+---+
     *  \endcode
     *
     *  \code
     *  // Truth vector:
     *  [0 1 2 1 1 2 1 1 2 1 1 2]
     *  \endcode
     *
     *  \code
     *  // Teddy code:
     *  teddy::ifmdd_manager<3> manager(3, 100, {2, 2, 3});
     *  std::vector<int> vec {0, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2};
     *  diagram_t f = manager.from_vector(vec);
     *  \endcode
     *
     *  \tparam I Iterator type for the input range.
     *  \tparam S Sentinel type for \p I (end iterator)
     *  \param manager Diagram manager
     *  \param first iterator to the first element of the truth vector
     *  \param last sentinel for \p first (end iterator)
     *  \return Diagram representing function given by the truth vector
     */
    template<
        class Data,
        class Degree,
        class Domain,
        std::input_iterator I,
        std::sentinel_for<I> S
    >
    static auto from_vector (
        diagram_manager<Data, Degree, Domain>& manager,
        I first,
        S last
    ) -> diagram_manager<Data, Degree, Domain>::diagram_t;

    /**
     *  \brief Creates diagram from a truth vector of a function
     *
     *  See the other overload for details.
     *
     *  \tparam R Type of the range that contains the truth vector
     *  \param vector Range representing the truth vector
     *  Elements of the range must be convertible to int
     *  \return Diagram representing function given by the truth vector
     */
    template<
        class Data,
        class Degree,
        class Domain,
        std::ranges::input_range R
    >
    static auto from_vector (
        diagram_manager<Data, Degree, Domain>& manager,
        R const& vector
    ) -> diagram_manager<Data, Degree, Domain>::diagram_t;

    /**
     *  \brief Creates truth vector from the diagram
     *
     *  Significance of variables is the same as in the \c from_vector
     *  function i.e., variable on the last level of the diagram is
     *  least significant. The following assertion holds:
     *  \code
     *  assert(manager.from_vector(manager.to_vector(d)))
     *  \endcode
     *
     *  \param manager Diagram manager
     *  \param diagram Diagram
     *  \return Vector of ints representing the truth vector
     */
    template<class Data, class Degree, class Domain>
    static auto to_vector (
        diagram_manager<Data, Degree, Domain> const& manager,
        diagram_manager<Data, Degree, Domain>::diagram_t const& diagram
    ) -> std::vector<int32>;

    /**
     *  \brief Creates truth vector from the diagram
     *  \tparam O Output iterator type
     *  \param manager Diagram manager
     *  \param diagram Diagram
     *  \param out Output iterator that is used to output the truth vector
     */
    template<
        class Data,
        class Degree,
        class Domain,
        std::output_iterator<teddy::int32> O
    >
    static auto to_vector_g (
        diagram_manager<Data, Degree, Domain> const& manager,
        diagram_manager<Data, Degree, Domain>::diagram_t const& diagram,
        O out
    ) -> void;

    /**
     *  \brief Prints dot representation of the graph
     *
     *  Prints dot representation of the entire multi rooted graph to
     *  the output stream.
     *
     *  \param manager Diagram manager
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     */
    template<class Data, class Degree, class Domain>
    static auto to_dot (
        diagram_manager<Data, Degree, Domain> const& manager,
        std::ostream& out
    ) -> void;

    /**
     *  \brief Prints dot representation of the diagram
     *
     *  Prints dot representation of the diagram to
     *  the output stream.
     *
     *  \param manager Diagram manager
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     *  \param diagram Diagram
     */
    template<class Data, class Degree, class Domain>
    static auto to_dot (
        std::ostream& out,
        diagram_manager<Data, Degree, Domain> const& manager,
        diagram_manager<Data, Degree, Domain>::diagram_t const& diagram
    ) -> void;

};

// definitions:

inline auto io::from_pla(
    bdd_manager& manager,
    pla_file const& file,
    fold_type const foldType
) -> std::vector<bdd_manager::diagram_t>
{
    using bdd_t = bdd_manager::diagram_t;
    auto const product = [&manager] (auto const& cube)
    {
        std::vector<bdd_t> variables;
        variables.reserve(as_usize(cube.size()));
        for (int32 i = 0; i < cube.size(); ++i)
        {
            if (cube.get(i) == 1)
            {
                variables.push_back(manager.variable(i));
            }
            else if (cube.get(i) == 0)
            {
                variables.push_back(manager.variable_not(i));
            }
        }
        return manager.left_fold<ops::AND>(variables);
    };

    auto const orFold = [&manager, foldType] (auto& diagrams)
    {
        switch (foldType)
        {
        case fold_type::Left:
            return manager.left_fold<ops::OR>(diagrams);

        case fold_type::Tree:
            return manager.tree_fold<ops::OR>(diagrams);

        default:
            assert(false);
            return manager.constant(0);
        }
    };

    auto const& plaLines      = file.get_lines();
    int64 const lineCount     = file.get_line_count();
    int64 const functionCount = file.get_function_count();

    // Create a diagram for each function.
    std::vector<bdd_t> functionDiagrams;
    functionDiagrams.reserve(as_usize(functionCount));
    for (int32 fi = 0; fi < functionCount; ++fi)
    {
        // First create a diagram for each product.
        std::vector<bdd_t> products;
        // products.reserve(lineCount);
        for (int32 li = 0; li < lineCount; ++li)
        {
            // We are doing SOP so we are only interested
            // in functions with value 1.
            if (plaLines[as_usize(li)].fVals_.get(fi) == 1)
            {
                products.push_back(
                    product(plaLines[as_uindex(li)].cube_)
                );
            }
        }

        // In this case we just have a constant function.
        if (products.empty())
        {
            products.push_back(manager.constant(0));
        }

        // Then merge products using OR.
        functionDiagrams.push_back(orFold(products));
    }

    return functionDiagrams;
}

template<
    class Data, class Degree,
    class Domain,
    std::input_iterator I,
    std::sentinel_for<I> S
>
auto io::from_vector (
    diagram_manager<Data, Degree, Domain>& manager,
    I first,
    S last
) -> diagram_manager<Data, Degree, Domain>::diagram_t
{
    return manager.from_vector(first, last);
}

template<class Data, class Degree, class Domain, std::ranges::input_range R>
auto io::from_vector(
    diagram_manager<Data, Degree, Domain>& manager,
    R const& vector
) -> diagram_manager<Data, Degree, Domain>::diagram_t
{
    return io::from_vector(manager, begin(vector), end(vector));
}

template<class Data, class Degree, class Domain>
auto io::to_vector (
    diagram_manager<Data, Degree, Domain> const& manager,
    diagram_manager<Data, Degree, Domain>::diagram_t const& diagram
) -> std::vector<int32>
{
    std::vector<int32> vector;
    vector.reserve(
        as_usize(manager.nodes_.domain_product(0, manager.get_var_count()))
    );
    io::to_vector_g(manager, diagram, std::back_inserter(vector));
    return vector;
}

template<
    class Data,
    class Degree,
    class Domain,
    std::output_iterator<teddy::int32> O
>
auto io::to_vector_g (
    diagram_manager<Data, Degree, Domain> const& manager,
    diagram_manager<Data, Degree, Domain>::diagram_t const& diagram,
    O out
) -> void
{
    if (manager.get_var_count() == 0)
    {
        assert(diagram.unsafe_get_root()->is_terminal());
        *out++ = diagram.unsafe_get_root()->get_value();
        return;
    }

    std::vector<int32> vars(as_usize(manager.get_var_count()));
    bool wasLast = false;
    do
    {
        *out++        = manager.evaluate(diagram, vars);
        bool overflow = true;
        int32 level   = manager.nodes_.get_leaf_level();
        while (level > 0 && overflow)
        {
            --level;
            int32 const index = manager.nodes_.get_index(level);
            ++vars[as_uindex(index)];
            overflow
                = vars[as_uindex(index)] == manager.nodes_.get_domain(index);
            if (overflow)
            {
                vars[as_uindex(index)] = 0;
            }

            wasLast = overflow && 0 == level;
        }
    } while (not wasLast);
}
}
#endif