#ifndef MIX_UTILS_LESS_FUNCTIONAL_HPP
#define MIX_UTILS_LESS_FUNCTIONAL_HPP

namespace teddy::utils
{
    /**
     *  @brief Creates a function that negates an output of binary function @p f .
     */
    auto constexpr fnot = [](auto const f)
    {
        return [f](auto const l, auto const r)
        {
            return ! f (l, r);
        };
    };

    /**
     *  @brief Modular addition.
     */
    template<std::size_t M>
    auto constexpr plus_mod = [](auto const l, auto const r) { return (l + r) % M; };
    template<std::size_t M>
    struct plus_mod_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return (l + r) % M;
        }
    };

    /**
     *  @brief Modular multiplication.
     */
    template<std::size_t M>
    auto constexpr multiplies_mod = [](auto const l, auto const r) { return (l * r) % M; };
    template<std::size_t M>
    struct multiplies_mod_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return (l * r) % M;
        }
    };

    /**
     *  @brief Logical and.
     */
    auto constexpr logical_and = [](auto const l, auto const r) { return l && r; };
    struct logical_and_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l && r;
        }
    };

    /**
     *  @brief Logical or.
     */
    auto constexpr logical_or = [](auto const l, auto const r) { return l || r; };
    struct logical_or_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l || r;
        }
    };

    /**
     *  @brief Logical nand.
     */
    auto constexpr logical_nand = fnot(logical_and);
    struct logical_nand_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return !(l && r);
        }
    };

    /**
     *  @brief Logical nor.
     */
    auto constexpr logical_nor = fnot(logical_or);
    struct logical_nor_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return !(l || r);
        }
    };

    /**
     *  @brief Logical xor.
     */
    auto constexpr logical_xor = [](auto const l, auto const r) { return l != r; };
    struct logical_xor_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l != r;
        }
    };

    /**
     *  @brief Equal to.
     */
    auto constexpr equal_to = [](auto const l, auto const r) { return l == r; };
    struct equal_to_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l == r;
        }
    };

    /**
     *  @brief Not equal to.
     */
    auto constexpr not_equal_to = fnot(equal_to);
    struct not_equal_to_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l != r;
        }
    };

    /**
     *  @brief Less.
     */
    auto constexpr less = [](auto const l, auto const r) { return l < r; };
    struct less_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l < r;
        }
    };

    /**
     *  @brief Less equal.
     */
    auto constexpr less_equal = [](auto const l, auto const r) { return l <= r; };
    struct less_equal_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l <= r;
        }
    };

    /**
     *  @brief Greater.
     */
    auto constexpr greater = [](auto const l, auto const r){ return l > r; };
    struct greater_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l > r;
        }
    };

    /**
     *  @brief Greater equal.
     */
    auto constexpr greater_equal = [](auto const l, auto const r){ return l >= r; };
    struct greater_equal_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l >= r;
        }
    };

    /**
     *  @brief Min.
     */
    auto constexpr min = [](auto const l, auto const r) { return l < r ? l : r; };
    struct min_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l < r ? l : r;
        }
    };

    /**
     *  @brief Max.
     */
    auto constexpr max = [](auto const l, auto const r) { return l > r ? l : r; };
    struct max_t
    {
        template<class T>
        auto operator () (T const l, T const r) const -> T
        {
            return l < r ? r : l;
        }
    };

    /**
     *  @brief Function that does nothing.
     */
    auto constexpr no_op = [](auto&&...) noexcept {};
    struct no_op_t
    {
        auto operator () () const -> void
        {
        }
    };

    /**
     *  @brief Returns true if given container is not empty.
     */
    auto constexpr not_empty = [](auto const& c) { return !c.empty(); };
    struct not_empty_t
    {
        template<class C>
        auto operator () (C const& c) const -> bool
        {
            return !c.empty();
        }
    };

    /**
     *  @brief Identity function for small value types.
     */
    auto constexpr identity = [](auto const a) { return a; };
    struct identity_t
    {
        template<class T>
        auto operator () (T t) const -> T
        {
            return t;
        }
    };

    /**
     *  @brief Creates a constant function that returns @p x by value.
     */
    auto constexpr constant = [](auto const x){ return [x](auto&&...){ return x; }; };
}

#endif