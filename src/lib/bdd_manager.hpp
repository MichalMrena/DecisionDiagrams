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
        using double_v   = std::vector<double>;
        using vertex_t   = typename base::vertex_t;

    /* Constructors */
    public:
        bdd_manager (std::size_t const varCount);

    /* Tools */
    public:
        auto satisfy_count (bdd_t& d) -> std::size_t;

        template< class VariableValues
                , class OutputIt
                , class SetVarVal = set_var_val<VariableValues> >
        auto satisfy_all (bdd_t const& d, OutputIt out) const -> void;

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
        auto calculate_probabilities    (bdd_t& f, double_v const& ps)     -> void;
        auto get_availability           () const                           -> double;
        auto get_unavailability         () const                           -> double;
        auto availability               (bdd_t& f, double_v const& ps)     -> double;
        auto unavailability             (bdd_t& f, double_v const& ps)     -> double;
        auto dpbd                       (bdd_t const& f, index_t const i)  -> bdd_t;
        auto dpbds                      (bdd_t const& f)                   -> bdd_v;
        auto structural_importance      (bdd_t& dpbd)                      -> double;
        auto structural_importances     (bdd_v& dpbds)                     -> double_v;
        auto birnbaum_importance        (bdd_t& dpbd,  double_v const& ps) -> double;
        auto birnbaum_importances       (bdd_v& dpbds, double_v const& ps) -> double_v;
        auto criticality_importance     (double const BI, double const qi, double const U)        -> double;
        auto criticality_importances    (double_v const& BIs, double_v const& ps, double const U) -> double_v;
        auto fussell_vesely_importance  (bdd_t& dpbd, double const qi, double_v const& ps, double const U) -> double;
        auto fussell_vesely_importances (bdd_v& dpbds, double_v const& ps, double const U)                 -> double_v;

        template<class VectorType>
        auto mcvs (std::vector<bdd_t> dpbds) -> std::vector<VectorType>;

    /* Internal aliases */
    private:
        using prob_table = typename base::prob_table;
        using trans_id_t = typename base::trans_id_t;
        using son_a      = typename base::son_a;

    /* Tools internals */
    private:
        template< class VariableValues
                , class OutputIt
                , class SetVarVal = set_var_val<VariableValues> >
        auto satisfy_all_step ( index_t   const i
                              , vertex_t* const v
                              , VariableValues& xs
                              , OutputIt&       out ) const -> void;

    /* Creation internals */
    private:
        auto line_to_product (pla_line const& line)   -> bdd_t;
        auto or_merge        (bdd_v diagrams, fold_e) -> bdd_t;

    /* Reliability internals */
    public:
        auto to_prob_table  (double_v const& ps) -> prob_table;
        auto to_mnf         (bdd_t const& dpbd)  -> bdd_t;
        auto to_dpbd_e      (bdd_t const& dpbd, index_t const i) -> bdd_t;
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
#include "diagrams/bdd_manager_tools.ipp"

#endif