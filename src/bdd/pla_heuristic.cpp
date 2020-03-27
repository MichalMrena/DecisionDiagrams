#include "pla_heuristic.hpp"

#include <algorithm>
#include <utility>
#include <limits>
#include "pla_file.hpp"

namespace mix::dd
{
    namespace
    {
        using metric_t       = double;
        using index_iterator = std::vector<index_t>::iterator;
        using swap_pairs_v   = std::vector<std::pair<index_t, index_t>>;

        struct mapper
        {
            std::pair<index_t, index_t> p;

            mapper(std::pair<index_t, index_t> pp) : p {pp} {};

            auto operator() (const index_t i) const -> index_t
            {
                if      (p.first  == i) return p.second;
                else if (p.second == i) return p.first;
                else return i;
            }
        };

        struct map_compare
        {
            const mapper& map;

            auto operator() (const index_t lhs, const index_t rhs) const -> bool
            {
                return map(lhs) < map(rhs);
            }
        };

        auto products_sizes (const pla_file& file) -> std::vector<size_t>
        {
            std::vector<size_t> sizes;
            sizes.reserve(file.line_count());

            for (const auto& line : file.get_lines())
            {
                size_t varCount {0};

                for (const auto var : line.cube)
                {
                    if (X != var)
                    {
                        ++varCount;
                    }
                }

                sizes.push_back(varCount);
            }

            return sizes;
        }

        auto sop_as_indices (const pla_file& file) -> std::vector<index_t>
        {
            std::vector<index_t> indices;
            indices.reserve(file.line_count() * file.variable_count());

            for (const auto& line : file.get_lines())
            {
                index_t index {0};
                for (const auto var : line.cube)
                {
                    if (X != var)
                    {
                        indices.push_back(index);
                    }
                    ++index;
                }
            }

            return indices;
        }

        auto all_swap_pairs (const std::vector<index_t>& is) -> swap_pairs_v
        {
            const auto n {is.size()};
            swap_pairs_v pairs;
            pairs.reserve((n * (n - 1)) >> 1);

            for (size_t i {0}; i < n; i++)
            {
                for (size_t j {i + 1}; j < n; j++)
                {
                    pairs.emplace_back(i, j);
                }
            }

            return pairs;  
        }

        auto product_metric ( index_iterator it
                            , index_iterator end
                            , const mapper& map ) -> metric_t
        {
            auto metric {0u};
            auto next   {it + 1};

            std::sort(it, end, map_compare {map});

            while (next != end)
            {
                metric = std::max(metric, map(*next) - map(*it) - 1);
                ++it;
                ++next;
            }

            return metric * metric;
        }

        auto total_metric ( std::vector<index_t>& sopIndices
                          , const std::vector<size_t>& prodSizes
                          , const mapper& map ) -> metric_t
        {
            auto productIt {sopIndices.begin()};
            auto maxesSum  {0.0};

            for (auto prodSize : prodSizes)
            {
                auto productEnd {productIt + prodSize};
                maxesSum += product_metric(productIt, productEnd, map);
                productIt = productEnd;
            }

            return maxesSum / prodSizes.size();
        }

        auto swap_indices ( std::vector<index_t>& sopIndices
                          , const mapper& map ) -> void
        {
            for (auto& index : sopIndices)
            {
                index = map(index);
            }
        }

        auto apply_swaps ( const swap_pairs_v& swaps
                         , pla_file& file ) -> void
        {
            for (const auto& swap : swaps)
            {
                file.swap_vars(swap.first, swap.second);
            }
        }

        auto verify_changes ( const pla_file& original
                            , swap_pairs_v swaps
                            , pla_file changed ) -> bool
        {
            std::reverse(swaps.begin(), swaps.end());
            apply_swaps(swaps, changed);

            return original == changed;
        }
    }

    auto improve_ordering (pla_file& file) -> pla_file&
    {
        const auto productSizes {products_sizes(file)};
        auto sopIndices         {sop_as_indices(file)};
        auto possibleSwaps      {all_swap_pairs(file.get_indices())}; // TODO nestačil by počet premenných?
        auto currentMetric      {total_metric(sopIndices, productSizes, std::make_pair(0u, 0u))};

        swap_pairs_v swaps;

        while (! possibleSwaps.empty())
        {
            const auto swapsEnd   {possibleSwaps.end()};
            auto swapsIt          {possibleSwaps.begin()};
            auto bestSwapIterator {possibleSwaps.begin()};
            auto proposedMetric   {currentMetric};

            while (swapsIt != swapsEnd)
            {
                const auto possibleMetric {total_metric(sopIndices, productSizes, *swapsIt)};

                if (possibleMetric < proposedMetric)
                {
                    proposedMetric   = possibleMetric;
                    bestSwapIterator = swapsIt;
                }

                ++swapsIt;
            }

            if (currentMetric <= proposedMetric)
            {
                break;
            }

            currentMetric = proposedMetric;

            swap_indices(sopIndices, *bestSwapIterator);
            swaps.push_back(*bestSwapIterator);

            std::iter_swap(bestSwapIterator, possibleSwaps.end() - 1);
            possibleSwaps.pop_back();
        }

        // const auto original {file};        
        apply_swaps(swaps, file);

        // if (! verify_changes(original, swaps, file))
        // {
        //     throw std::runtime_error {"Not good."};
        // }

        return file;
    }
}