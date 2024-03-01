#include <libteddy/core_io.hpp>
#include <libteddy/reliability.hpp>

auto main() -> int
{
    using namespace teddy;
    using namespace teddy::ops;
    using bdd = bss_manager::diagram_t;
    bss_manager manager(2, 1'000);
    auto& x = manager;
    bdd f = manager.apply<AND>(x(0), x(1));
    bdd uf = manager.dp_unpack({{0,1}}, f);
    manager.to_dot_graph(std::cout, uf);
}