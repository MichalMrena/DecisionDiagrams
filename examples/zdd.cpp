#include <libteddy/impl/zdd_manager.hpp>
#include <libteddy/inc/core.hpp>
void static_creation() {
    std::vector<int> v = {0,0,0,1,1,0,0,1};
    std::vector<double> counts;

    teddy::zdd_manager manager(3, 1000, 100);

    auto diagram = manager.from_vector(v);
    manager.to_dot(diagram);
}

auto dynamic_creation() -> void {
    auto mgr = teddy::zdd_manager(3, 1000, 100);

    auto result = mgr.empty();

    auto s1 = mgr.base();      // {}
    s1 = mgr.change(s1, 1);    // {1}
    s1 = mgr.change(s1, 2);    // {1,2}

    auto s2 = mgr.base();      // {}
    s2 = mgr.change(s2, 0);    // {0}

    auto s3 = mgr.base();      // {}
    s3 = mgr.change(s3, 0);    // {0}
    s3 = mgr.change(s3, 1);    // {0,1}
    s3 = mgr.change(s3, 2);    // {0,1,2}

    result = mgr.unification(result, s1);
    result = mgr.unification(result, s2);
    result = mgr.unification(result, s3);

    mgr.to_dot(result);
}

int main() { //NOLINT
    dynamic_creation();

    return 0;
}
