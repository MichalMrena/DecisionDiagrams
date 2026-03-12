#include <libteddy/impl/zdd_manager.hpp>
#include <libteddy/inc/core.hpp>


int main()
{
    teddy::zdd_manager manager(3, 1000, 100);
    std::vector<int> v = {0,0,0,1,1,0,0,1};
    
    auto* d = manager.from_vector(v);

    return 0;
}
