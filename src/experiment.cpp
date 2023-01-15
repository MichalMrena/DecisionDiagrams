#include <libteddy/teddy.hpp>
#include <algorithm>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <fstream>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_set>
#include <tuple>
#include <variant>

#include "iterators.hpp"

using teddy::int32;
using teddy::int64;
using teddy::as_usize;

enum class Op
{
    And,
    Or,
    Undefined
};

inline static auto Ops = {Op::And, Op::Or};

auto get_neutral_element(Op const o)
{
    switch (o)
    {
        case Op::And: return std::numeric_limits<int32>::max();
        case Op::Or:  return std::numeric_limits<int32>::min();
        default:      std::terminate(); return int32{};
    }
}

// LeafNode:

struct LeafNode
{
    int32 index_;
};

// Forward declarations:

struct BinaryNode;
struct MultiwayNode;

// BinOpNode:

struct BinOpNode
{
    Op op_;
    BinaryNode const* lhs_;
    BinaryNode const* rhs_;

    auto evaluate(auto const l, auto const r) const -> int32
    {
        switch (op_)
        {
        case Op::And:
            return std::min(l, r);

        case Op::Or:
            return std::max(l, r);

        default:
            std::terminate(); return int32{};
        }
    }
};

// NAryOpNode:

struct NAryOpNode
{
    Op op_;
    std::vector<MultiwayNode*> args_;

    auto evaluate(auto const& intArgs) const
    {
        switch (op_)
        {
            case Op::And:
                return std::reduce(
                    begin(intArgs),
                    end(intArgs),
                    get_neutral_element(op_),
                    teddy::ops::MIN()
                );

            case Op::Or:
                return std::reduce(
                    begin(intArgs),
                    end(intArgs),
                    get_neutral_element(op_),
                    teddy::ops::MAX()
                );

            default:
                default: std::terminate(); return int32{};
        }
    }
};

// BinaryNode:

struct BinaryNode
{
    std::variant<std::monostate, LeafNode, BinOpNode> data_;

    auto is_variable() const
    {
        return std::holds_alternative<LeafNode>(data_);
    }

    auto is_constant() const
    {
        return false;
    }

    auto is_operation() const
    {
        return std::holds_alternative<BinOpNode>(data_);
    }

    auto get_index() const
    {
        return std::get<LeafNode>(data_).index_;
    }

    auto get_value() const
    {
        return int32{};
    }

    auto get_operation() const
    {
        return std::get<BinOpNode>(data_).op_;
    }

    auto evaluate(int32 const l, int32 const r) const
    {
        return std::get<BinOpNode>(data_).evaluate(l, r);
    }

    auto get_left() const -> BinaryNode const&
    {
        return *std::get<BinOpNode>(data_).lhs_;
    }

    auto get_right() const -> BinaryNode const&
    {
        return *std::get<BinOpNode>(data_).rhs_;
    }
};

static_assert(
    teddy::expression_node<BinaryNode>,
    "BinaryNode satisfies expression_node"
);

// F :: BinaryNode const& -> Int -> Int -> ()
template<class F>
auto for_each_dfs (BinaryNode const& root, F f) -> void
{
    auto nextId = 0;
    auto go = [&nextId, f](
        auto self,
        BinaryNode const& node,
        int const parentId
    ) -> void
    {
        auto const thisId = nextId;
        ++nextId;
        std::invoke(f, node, parentId, thisId);
        if (node.is_operation())
        {
            self(self, node.get_left(), thisId);
            self(self, node.get_right(), thisId);
        }
    };
    go(go, root, -1);
}

// MultiwayNode:

struct MultiwayNode
{
    std::variant<std::monostate, LeafNode, NAryOpNode> data_;

    auto is_variable() const
    {
        return std::holds_alternative<LeafNode>(data_);
    }

    auto is_operation() const
    {
        return std::holds_alternative<NAryOpNode>(data_);
    }

    auto get_index() const
    {
        return std::get<LeafNode>(data_).index_;
    }

    auto get_operation() const
    {
        return std::get<NAryOpNode>(data_).op_;
    }

    auto evaluate(auto const& intArgs) const
    {
        return std::get<NAryOpNode>(data_).evaluate(intArgs);
    }

    auto get_args() const -> std::vector<MultiwayNode*> const&
    {
        return std::get<NAryOpNode>(data_).args_;
    }

    auto as_opnode() const -> NAryOpNode const&
    {
        return std::get<NAryOpNode>(data_);
    }

    auto as_leafnode() const -> LeafNode const&
    {
        return std::get<LeafNode>(data_);
    }
};

using MwNodeHash = decltype([](MultiwayNode const& node)
{
    if (node.is_variable())
    {
        return std::size_t{1};
    }
    else
    {
        auto result = std::size_t{0};
        for (auto const* son : node.get_args())
        {
            auto const hash = std::hash<MultiwayNode const*>()(son);
            result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return result;
    }
});

using MwNodeEquals = decltype([](MultiwayNode const& l, MultiwayNode const& r)
{
    if (l.is_variable() && r.is_variable())
    {
        return true;
    }
    else if ((l.is_variable() && r.is_operation())
          || (r.is_operation() && l.is_variable()))
    {
        return false;
    }
    else
    {
        return l.get_operation() == r.get_operation()
            && std::ranges::equal(l.get_args(), r.get_args());
    }
});

// F :: MultiwayNode const& -> Int -> Int -> ()
template<class F>
auto for_each_dfs(MultiwayNode const& root, F f)
{
    auto nextId = 0;
    auto go = [&nextId, f](
        auto self,
        MultiwayNode const& node,
        int const parentId
    ) -> void
    {
        auto const thisId = nextId;
        ++nextId;
        std::invoke(f, node, parentId, thisId);
        if (node.is_operation())
        {
            for (auto* son : node.get_args())
            {
                self(self, *son, thisId);
            }
        }
    };
    go(go, root, -1);
}

// Graphviz utils:

// dump_dot :: BinaryNode | MultiwayNode -> std::string
auto dump_dot (auto const& root) -> std::string
{
    auto out = std::string();
    out += "digraph BinTree {\n";

    auto s = [](auto const x){ return std::to_string(x); };

    for_each_dfs(root, [&](
        auto const& node,
        int const,
        int const nodeId
    )
    {
        auto const label = node.is_variable()
            ? "x"
            : (node.get_operation() ==Op::And ? "and" : "or");
        out += "    " + s(nodeId) + " [label=\"" + label + "\"];\n";
    });
    out += "\n";

    for_each_dfs(root, [&out, s](
        auto const&,
        int const parentId,
        int const nodeId
    )
    {
        if (parentId != -1)
        {
            out += "    "
                + s(parentId)
                + " -> "
                + s(nodeId) + ";\n";
        }
    });
    out += "}\n";

    return out;
}

// BinAstGenerator:

class BinAstGenerator
{
public:
    BinAstGenerator(int32 leafcount, int32 nextvar) :
        leafcount_ (leafcount),
        nextvar_   (nextvar),
        lhssizes_  (teddy::utils::fill_vector(
            leafcount / 2,
            [](auto const s){ return s + 1; }
        )),
        opsit_(begin(Ops)),
        lhssizesit_(begin(lhssizes_))
    {
        this->reset_lhsgen();
        this->reset_rhsgen();
        this->make_tree();
    }

    auto get () const -> BinaryNode const&
    {
        return *node_;
    }

    auto advance () -> void
    {
        this->advance_state();
        this->make_tree();
    }

    auto is_done () const -> bool
    {
        return opsit_ == end(Ops);
    }

private:
    auto advance_state () -> void
    {
        if (1 == leafcount_)
        {
            opsit_ = end(Ops);
        }
        else
        {
            auto resetrhsgen  = false;
            auto resetlhsgen  = false;
            auto resetlhssize = false;

            rhsgen_->advance();
            if (rhsgen_->is_done())
            {
                resetrhsgen = true;
                lhsgen_->advance();
                if (lhsgen_->is_done())
                {
                    resetlhsgen = true;
                    ++lhssizesit_;
                    if (lhssizesit_ == end(lhssizes_))
                    {
                        resetlhssize = true;
                        ++opsit_;
                    }
                }
            }

            if (opsit_ == end(Ops))
            {
                return;
            }

            if (resetlhssize)
            {
                this->reset_lhssizeit();
            }

            if (resetlhsgen)
            {
                this->reset_lhsgen();
            }

            if (resetrhsgen)
            {
                this->reset_rhsgen();
            }
        }
    }

    auto make_tree () -> void
    {
        if (1 == leafcount_)
        {
            node_ = std::make_unique<BinaryNode>(
                BinaryNode {LeafNode {nextvar_}}
            );
        }
        else
        {
            auto const& lhs = lhsgen_->get();
            auto const& rhs = rhsgen_->get();
            node_ = std::make_unique<BinaryNode>(
                BinaryNode {BinOpNode {*opsit_, &lhs, &rhs}}
            );
        }
    }

    auto reset_lhsgen () -> void
    {
        if (leafcount_ > 1)
        {
            lhsgen_ = std::make_unique<BinAstGenerator>(
                *lhssizesit_,
                nextvar_
            );
        }
    }

    auto reset_rhsgen () -> void
    {
        if (leafcount_ > 1)
        {
            rhsgen_ = std::make_unique<BinAstGenerator>(
                leafcount_ - *lhssizesit_,
                *lhssizesit_ + nextvar_
            );
        }
    }

    auto reset_lhssizeit () -> void
    {
        lhssizesit_ = begin(lhssizes_);
    }

private:
    int32 leafcount_;
    int32 nextvar_;
    std::vector<int32> lhssizes_; // TODO to iota range

    decltype(Ops)::iterator opsit_;
    std::vector<int32>::iterator lhssizesit_;

    std::unique_ptr<BinAstGenerator> lhsgen_;
    std::unique_ptr<BinAstGenerator> rhsgen_;

    std::unique_ptr<BinaryNode> node_;
};

// SonVarCountsGenerator:

class SonVarCountsGenerator
{
public:
    SonVarCountsGenerator(int32 const parentVarCount) :
        counts_({}) // TODO
    {
    }

    auto get () const -> std::vector<int32> const&
    {
        return counts_;
    }

    auto advance () -> void
    {
        // TODO
    }

    auto is_done () const -> bool
    {
        return empty(counts_);
    }

private:
    std::vector<int32> counts_;
};

// MwAstGenerator:

class MwAstGenerator
{
public:
    MwAstGenerator(
        int32 const varCount,
        std::unordered_map<
            MultiwayNode,
            MultiwayNode*,
            MwNodeHash,
        MwNodeEquals>& uniqueTable
    ) :
        sonVarCountsGenerator_ (varCount),
        uniqueTable_           (&uniqueTable)
    {
    }

    auto get () const -> MultiwayNode*
    {
        return currentTree_;
    }

    auto advance () -> void
    {
        this->advance_state();
        this->make_tree();
    }

    auto reset () -> void
    {
        // TODO this->reset_son_var_count_generator(this->get_var_count());
        this->reset_son_generators();
    }

    auto is_done () const -> bool
    {
        return sonVarCountsGenerator_.is_done();
    }

private:

    auto advance_state () -> void
    {
        auto overflow = false;
        for (auto& sonGenerator : sonGenerators_)
        {
            sonGenerator->advance();
            if (sonGenerator->is_done())
            {
                overflow = true;
                sonGenerator->reset();
            }
            else
            {
                break;
            }
        }

        if (overflow)
        {
            sonVarCountsGenerator_.advance();
            this->reset_son_generators();
        }
    }

    auto make_tree () -> void
    {
        auto key = MultiwayNode{};

        if (this->get_var_count() == 1)
        {
            key.data_ = LeafNode{0};
        }
        else
        {
            auto sons = std::vector<MultiwayNode*>();
            sons.reserve(size(sonGenerators_));
            for (auto const& sonGenUPtr : sonGenerators_)
            {
                sons.push_back(sonGenUPtr->get());
            }

            key.data_ = NAryOpNode{
                Op::Undefined,
                std::move(sons)
            };
        }

        auto it  = uniqueTable_->find(key);
        if (it == end(*uniqueTable_))
        {
            auto* newNode = new MultiwayNode{key};
            uniqueTable_->try_emplace(key, newNode);
            currentTree_ = newNode;
        }
        else
        {
            currentTree_ = it->second;
        }
    }

    auto reset_son_generators () -> void
    {
        sonGenerators_.clear();
        auto const& sonVarCounts = sonVarCountsGenerator_.get();
        for (auto const sonVarCount : sonVarCounts)
        {
            sonGenerators_.emplace_back(
                std::make_unique<MwAstGenerator>(sonVarCount, *uniqueTable_)
            );
        }
    }

    auto get_var_count () const -> int32
    {
        return static_cast<int32>(ssize(sonGenerators_));
    }

private:
    SonVarCountsGenerator sonVarCountsGenerator_;
    std::vector<std::unique_ptr<MwAstGenerator>> sonGenerators_;
    std::unordered_map<
        MultiwayNode,
        MultiwayNode*,
        MwNodeHash,
    MwNodeEquals>* uniqueTable_;
    MultiwayNode* currentTree_;
};

template<int64 M, int64 N>
auto make_truth_vector (
    teddy::mdd_manager<M> const& manager,
    BinaryNode const& root
)
{
    auto eval_node = [](
        auto self,
        std::vector<int32> const& vs,
        BinaryNode const& n
    ) -> int32
    {
        if (n.is_operation())
        {
            auto const l = self(self, vs, n.get_left());
            auto const r = self(self, vs, n.get_right());
            return n.evaluate(l, r);
        }
        else
        {
            return vs[n.get_index()];
        }
    };

    auto const ds  = manager.get_domains();
    auto it        = domain_iterator(ds, manager.get_order());
    auto const end = domain_iterator_sentinel();
    auto vec       = std::bitset<as_usize(teddy::utils::int_pow(M, N))>();
    auto i         = 0ul;

    while (it != end)
    {
        vec[i] = eval_node(eval_node, *it, root);
        ++i;
        ++it;
    }

    return vec;
}

// F :: diagram -> ()
template<int64 M, int64 N, class F>
auto for_each_bin_ast (teddy::mdd_manager<M>& manager, F f)
{
    auto memo = std::unordered_set<void*>();
    auto gen  = BinAstGenerator(N, 0);
    auto i    = 1;
    while (not gen.is_done())
    {
        auto& root = gen.get();
        auto d = manager.from_expression_tree(root);

            std::cout << i << '\n';
            std::cout << dump_dot(root);
            std::cout << "---" << '\n';

        if (not memo.contains(d.get_root()))
        {
            std::invoke(f, d);
            memo.emplace(d.get_root());
        }
            ++i;

        gen.advance();
    }
}

auto bin_to_mw_ast (
    BinaryNode const& binRoot,
    auto& unique,
    auto* leaf
) -> MultiwayNode*
{
    auto transform = teddy::utils::fix([&](
        auto self,
        BinaryNode const& binNode
    ) -> MultiwayNode*
    {
        if (binNode.is_operation())
        {
            auto mappedSons = std::vector<MultiwayNode*>();
            mappedSons.push_back(self(self, binNode.get_left()));
            mappedSons.push_back(self(self, binNode.get_right()));
            return new MultiwayNode{NAryOpNode{
                binNode.get_operation(),
                std::move(mappedSons)
            }};
        }
        else
        {
            return leaf;
        }
    });

    auto reduce = teddy::utils::fix([](
        auto self,
        MultiwayNode* node
    ) -> MultiwayNode*
    {
        if (node->is_operation())
        {
            auto mappedSons = std::vector<MultiwayNode*>();
            for (auto* son : node->get_args())
            {
                mappedSons.push_back(self(self, son));
            }

            auto newSons = std::vector<MultiwayNode*>();
            for (auto* son : mappedSons)
            {
                if (son->is_operation()
                 && son->get_operation() == node->get_operation())
                {
                    auto& sonsSons = son->get_args();
                    newSons.insert(
                        end(newSons),
                        begin(sonsSons),
                        end(sonsSons)
                    );
                    delete son;
                }
                else
                {
                    newSons.push_back(son);
                }
            }
            std::ranges::sort(newSons);
            std::get<NAryOpNode>(node->data_).args_ = std::move(newSons);
        }
        return node;
    });

    auto uniquize = teddy::utils::fix([&unique](
        auto self,
        MultiwayNode* node
    ) -> MultiwayNode*
    {
        if (node->is_operation())
        {
            auto mappedSons = std::vector<MultiwayNode*>();
            for (auto* son : node->get_args())
            {
                mappedSons.push_back(self(self, son));
            }
            std::ranges::sort(mappedSons);

            auto newNodeKey = MultiwayNode{NAryOpNode{
                node->get_operation(),
                std::move(mappedSons)
            }};
            delete node;

            auto it = unique.find(newNodeKey);
            if (it == end(unique))
            {
                auto newNode = new MultiwayNode(newNodeKey);
                unique.try_emplace(newNodeKey, newNode);
                return newNode;
            }
            else
            {
                return it->second;
            }
        }
        else
        {
            return node;
        }
    });

    return uniquize(reduce(transform(binRoot)));
}

// F :: MultiwayNode const& -> ()
template<class F>
auto for_each_mw_ast (int32 const varCount, F f)
{
    auto unique = std::unordered_map<
        MultiwayNode,
        MultiwayNode*,
        MwNodeHash,
        MwNodeEquals
    >();

    auto leaf = new MultiwayNode{LeafNode{0}};
    auto memo = std::unordered_set<MultiwayNode*>();
    auto gen  = BinAstGenerator(varCount, 0);
    while (not gen.is_done())
    {
        auto& binRoot = gen.get();
        auto* mwRoot = bin_to_mw_ast(binRoot, unique, leaf);
        if (not memo.contains(mwRoot))
        {
            std::invoke(f, *mwRoot);
            memo.insert(mwRoot);
        }
        gen.advance();
    }

    for (auto const& [key, val] : unique)
    {
        delete val;
    }
    delete leaf;
}

auto count_trees (int32 const n) -> int64
{
    // https://oeis.org/A248748

    auto gen   = BinAstGenerator(n, 0);
    auto count = int64{0};

    while (not gen.is_done())
    {
        gen.advance();
        ++count;
    }

    return count;
}

template<int64 M>
auto create_series(
    teddy::mdd_manager<M>& manager,
    auto                   diagram
)
{
    auto series = std::vector<typename teddy::mdd_manager<M>::diagram_t>();
    series.reserve(M - 1);
    for (auto j = 1u; j < M; ++j)
    {
        auto seriesDiagram = manager.template apply<teddy::ops::GREATER_EQUAL>(
            diagram,
            manager.constant(j)
        );
        series.emplace_back(seriesDiagram);
    }
    return series;
}

template<int64 M, int64 N>
auto compare_series_parallel () -> void
{
    auto m = teddy::mdd_manager<M>(N, 1'000'000);

    auto singlebetter = int64{0};
    auto seriesbetter = int64{0};
    auto equalsize    = int64{0};

    for_each_bin_ast<M, N>(m, [&](auto const& single)
    {
        auto const series      = create_series<M>(m, single);
        auto const singlecount = m.node_count(single);
        auto const seriescount = std::invoke([&]()
        {
            auto unique = std::unordered_set<void*>();
            for (auto const& sd : series)
            {
                m.nodes_.traverse_pre(sd.get_root(), [&unique](auto const n)
                {
                    unique.insert(n);
                });
            }
            return std::size(unique);
        });
        singlebetter += static_cast<int64>(singlecount < seriescount);
        seriesbetter += static_cast<int64>(seriescount < singlecount);
        equalsize    += static_cast<int64>(seriescount == singlecount);
    });

    auto const total = (singlebetter + seriesbetter + equalsize);
    auto const relativeequal = static_cast<double>(equalsize)
                             / static_cast<double>(total);
    std::cout << singlebetter  << "\t"
              << seriesbetter  << "\t"
              << equalsize     << "\t"
              << relativeequal << '\n';
}

#define CMP(n) std::cout << n << "\t"; \
               compare_series_parallel<5, n>();

auto count_mw(int32 const varCount)
{
    auto count = int64{0};
    for_each_mw_ast(varCount, [&count](auto const&)
    {
        ++count;
    });
    std::cout << varCount << " \t" << count << '\n';
}

auto main () -> int
{
    // CMP(4);
    // CMP(5);
    // CMP(6);
    // CMP(7);
    // CMP(8);
    // CMP(9);
    // CMP(10);
    // CMP(11);
    // CMP(12);
    // CMP(13);
    // CMP(14);
    // CMP(15);

    for (auto i = 2; i < 11; ++i)
    {
        count_mw(i);
    }

    std::cout << "=== end of main ===" << '\n';
}
