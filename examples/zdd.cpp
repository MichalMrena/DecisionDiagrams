#include <libteddy/impl/zdd_manager.hpp>
#include <libteddy/inc/core.hpp>


int main() { //NOLINT
    
    //std::vector<int> v = {0,0,0,1,1,0,0,1};

    /*std::vector<int> v(32, 0);
      v[3]  = 1;  // 00011
      v[5]  = 1;  // 00101
      v[6]  = 1;  // 00110
      v[9]  = 1;  // 01001
      v[10] = 1;  // 01010
      v[12] = 1;  // 01100
      v[17] = 1;  // 10001
      v[21] = 1;  // 10101
      v[25] = 1;  // 11001
      v[31] = 1;  // 11111
    */

    std::vector<int> v = { 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1 };

    auto var_count = static_cast<int>(std::log2(v.size()));
    teddy::zdd_manager manager(var_count, 1000, 100);

    auto* d = manager.from_vector(v);

    manager.to_dot(d);

    return 0;
}
