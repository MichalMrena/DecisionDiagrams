#include <libteddy/core.hpp>
#include <libteddy/details/tools.hpp>
#include <chrono>
#include <iostream>
#include <optional>
#include <vector>

/**
 *  Implementation of the N-Queen problem adapted from Sylvan:
 *  https://github.com/utwente-fmt/sylvan/blob/master/examples/nqueens.c
 */
void solve(int const n)
{
    teddy::bdd_manager manager(n*n, 1'000'000);
    manager.set_cache_ratio(2);
    manager.set_gc_ratio(0.30);
    using bdd_t = teddy::bdd_manager::diagram_t;
    using namespace teddy::ops;

    std::vector<bdd_t> board;
    board.reserve(static_cast<std::size_t>(n * n));
    for (int i = 0; i < n * n; ++i)
    {
        board.push_back(manager.variable(i));
    }

    bdd_t result = manager.constant(1);

    // rows
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            bdd_t tmp = manager.constant(1);
            for (int k = 0; k < n; ++k){
                if (j != k)
                {
                    tmp = manager.apply<AND>(
                        tmp,
                        manager.negate(board[static_cast<std::size_t>(i*n + k)])
                    );
                }
            }
            tmp = manager.apply<OR>(
                tmp,
                manager.negate(board[static_cast<std::size_t>(i*n + j)])
            );
            result = manager.apply<AND>(result, tmp);
        }
    }

    // cols
    for (int j = 0; j < n; ++j)
    {
        for (int i = 0; i < n; ++i)
        {
            bdd_t tmp = manager.constant(1);
            for (int k = 0; k < n; ++k)
            {
                if (i != k)
                {
                    tmp = manager.apply<AND>(
                        tmp,
                        manager.negate(board[static_cast<std::size_t>(k*n + j)])
                    );
                }
            }
            tmp = manager.apply<OR>(
                tmp,
                manager.negate(board[static_cast<std::size_t>(i*n + j)])
            );
            result = manager.apply<AND>(result, tmp);
        }
    }

    // rising diagonals
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            bdd_t tmp = manager.constant(1);
            for (int k = 0; k < n; ++k)
            {
                if (j+k >= i && j+k < n+i && k != i)
                {
                    tmp = manager.apply<AND>(
                        tmp,
                        manager.negate(
                            board[static_cast<std::size_t>(k*n + (j+k-i))]
                        )
                    );
                }
            }
            tmp = manager.apply<OR>(
                tmp,
                manager.negate(board[static_cast<std::size_t>(i*n + j)])
            );
            result = manager.apply<AND>(result, tmp);
        }
    }

    // falling diagonals
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            bdd_t tmp = manager.constant(1);
            for (int k = 0; k < n; ++k)
            {
                if (j+i >= k && j+i < n+k && k != i)
                {
                    tmp = manager.apply<AND>(
                        tmp,
                        manager.negate(
                            board[static_cast<std::size_t>(k*n + (j+i-k))]
                        )
                    );
                }
            }
            tmp = manager.apply<OR>(
                tmp,
                manager.negate(board[static_cast<std::size_t>(i*n + j)])
            );
            result = manager.apply<AND>(result, tmp);
        }
    }

    // place queens
    for (int i = 0; i < n; ++i) {
        bdd_t tmp = manager.constant(0);
        for (int j = 0; j < n; ++j) {
            tmp = manager.apply<OR>(
                tmp,
                board[static_cast<std::size_t>(i*n + j)]
            );
        }
        result = manager.apply<AND>(result, tmp);
    }

    #ifdef LIBTEDDY_COLLECT_STATS
    std::cout << "===\n";
    teddy::dump_stats();
    std::cout << "===\n";
    #endif

    std::cout << "bdd node-count:      "
              << manager.get_node_count(result)
              << "\n";
    std::cout << "number of solutions: "
              << manager.satisfy_count(1, result)
              << "\n";
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Please specify the number of queens.\n";
        return 1;
    }

    const std::optional<int> nOpt = teddy::utils::parse<int>(argv[1]);
    if (not nOpt)
    {
        std::cerr << "Please specify the number of queens.\n";
        return 1;
    }
    const int n = *nOpt;

    const auto before = std::chrono::high_resolution_clock::now();
    solve(n);
    const auto after = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        after - before
    );
    std::cout << "elapsed time:        " << elapsed.count() << "ms" << "\n";

}