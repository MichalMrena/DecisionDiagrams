#ifndef MIX_DD_HALF_SYMBOLIC_F
#define MIX_DD_HALF_SYMBOLIC_F

#include <bitset>
#include "typedefs.hpp"
#include "bool_function.hpp"

namespace mix::dd
{
    template<size_t VariableCount, class Function>
    class half_symbolic_f : public bool_function
    {
    private:
        Function f;

    public:
        half_symbolic_f(Function pF);
        virtual ~half_symbolic_f() = default;

        auto operator[] (const input_t input) const -> log_val_t override;
        
        auto variable_count () const -> size_t override;

    private:
        auto transform_input (const input_t input) const -> std::bitset<VariableCount + 1>;
    };    

    template<size_t VariableCount, class Function>
    half_symbolic_f<VariableCount, Function>::half_symbolic_f(Function pF) :
        f {pF}
    {
    }

    template<size_t VariableCount, class Function>
    auto half_symbolic_f<VariableCount, Function>::operator[] 
        (const input_t input) const -> log_val_t
    {
        return this->f(this->transform_input(input));
    }

    template<size_t VariableCount, class Function>
    auto half_symbolic_f<VariableCount, Function>::variable_count 
        () const -> size_t
    {
        return VariableCount;
    }

    template<size_t VariableCount, class Function>
    auto half_symbolic_f<VariableCount, Function>::transform_input
        (const input_t input) const -> std::bitset<VariableCount + 1>
    {
        const std::bitset<VariableCount> inputBits {input};
        std::bitset<VariableCount + 1> transformedBits;

        for (size_t inputIndex {VariableCount}; inputIndex > 0;)
        {
            --inputIndex;
            transformedBits[VariableCount - inputIndex] = inputBits[inputIndex];
        }

        return transformedBits;
    }


    
    template<size_t VariableCount>
    using xs = const std::bitset<VariableCount + 1>&;

    template<size_t VariableCount, class Function>
    auto create_hs (Function f) -> half_symbolic_f<VariableCount, Function>
    {
        return half_symbolic_f<VariableCount, Function> {f};
    }
}

#endif