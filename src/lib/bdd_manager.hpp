#ifndef MIX_DD_BDD_MANAGER_HPP
#define MIX_DD_BDD_MANAGER_HPP

#include "mdd_manager.hpp"
#include "diagrams/pla_file.hpp"

namespace mix::dd
{
    enum class fold_e {left, tree};

    template<class VertexData, class ArcData>
    class bdd_manager : public mdd_manager<VertexData, ArcData, 2>
    {
    /* Public aliases */
    public:
        using base       = mdd_manager<VertexData, ArcData, 2>;
        using bdd_t      = typename base::mdd_t;
        using bdd_v      = std::vector<bdd_t>;
        using bool_var_v = std::vector<bool_var>;
        using prob_v     = std::vector<double>;

    /* Constructors */
    public:
        bdd_manager (std::size_t const varCount);

    /* Manipulation */
    public:
        auto negate (bdd_t const& d) -> bdd_t;

    /* Creation */
    public:
        auto just_var_not (index_t const i)        -> bdd_t;
        auto just_vars    (bool_var_v const& vars) -> bdd_v;
        auto from_pla     (pla_file const& file, fold_e const mm = fold_e::tree) -> bdd_v;

    /* Reliability */
    public:
        auto calculate_probabilities (bdd_t& f, prob_v const& ps) -> void;
        auto get_availability        () const                     -> double;
        auto get_unavailability      () const                     -> double;
        auto availability            (bdd_t& f, prob_v const& ps) -> double;
        auto unavailability          (bdd_t& f, prob_v const& ps) -> double;

    /* Internal aliases */
    private:
        using prob_table         = typename base::prob_table;
        using transformator_id_t = typename base::transformator_id_t;

    /* Creation internals */
    private:
        auto line_to_product (pla_line const& line)   -> bdd_t;
        auto or_merge        (bdd_v diagrams, fold_e) -> bdd_t;

    /* Reliability internals */
    private:
        auto to_prob_table (prob_v const& ps) -> prob_table;

    /* Static constants */
    private:
        inline static constexpr auto T_NEGATE = transformator_id_t {1};
    };

    template<class VertexData, class ArcData>
    bdd_manager<VertexData, ArcData>::bdd_manager
        (std::size_t const varCount) :
        base {varCount}
    {
    }
}

#include "diagrams/bdd_manager_creator.ipp"
#include "diagrams/bdd_manager_reliability.ipp"
#include "diagrams/bdd_manager_manipulator.ipp"

#endif