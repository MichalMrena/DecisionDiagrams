#ifndef MIX_DS_PEEKABLE_STACK_HPP
#define MIX_DS_PEEKABLE_STACK_HPP

#include <deque>
#include <stack>
#include <stdexcept>

namespace mix::ds
{
    /**
        @brief Extension of std::stack that allows you to peek elements under top() using peek(offset) method.

        They say that one does not simply inherit from std:: Container.
        However I've decided that this is the simplest approach to achieve the desired result.
        I don't see any situations when using this container would result in something bad.
        If you see a problem with it please let me know.
     */
    template<class T, class Container = std::deque<T>>
    class peekable_stack : public std::stack<T, Container>
    {
    public:
        using base            = std::stack<T, Container>;
        using container_type  = typename base::container_type;
        using value_type      = typename base::size_type;
        using reference       = typename base::reference;
        using const_reference = typename base::const_reference;
        using size_type       = typename base::size_type;

        template<class Alloc>
        using uses_allocator  = typename std::enable_if_t<std::uses_allocator<container_type, Alloc>::value>;

    public: // See the documentation for std::stack constructors.
        peekable_stack ();
        explicit peekable_stack (Container const& cont);
        explicit peekable_stack (Container&& cont);
        peekable_stack (peekable_stack const& other);
        peekable_stack (peekable_stack&& other);

        template<class Alloc, class = uses_allocator<Alloc>> explicit peekable_stack (Alloc const& alloc);
        template<class Alloc, class = uses_allocator<Alloc>> peekable_stack (Container const&, Alloc const& alloc);
        template<class Alloc, class = uses_allocator<Alloc>> peekable_stack (Container&&, Alloc const& alloc);
        template<class Alloc, class = uses_allocator<Alloc>> peekable_stack (peekable_stack const& other, Alloc const& alloc);
        template<class Alloc, class = uses_allocator<Alloc>> peekable_stack (peekable_stack&& other, Alloc const& alloc);

    public:
        /**
            @return reference to the element that is @p offset positions under the top().
         */
        auto operator[] (size_type const offset) -> reference;

        /**
            @return const_reference to the element that is @p offset positions under the top().
         */
        auto operator[] (size_type const offset) const -> const_reference;

        /**
            @return reference to the element that is @p offset positions under the top().
            @throw  std::out_of_range
         */
        auto peek (size_type const offset) -> reference;

        /**
            @return const_reference to the element that is @p offset positions under the top().
            @throw  std::out_of_range
         */
        auto peek (size_type const offset) const -> const_reference;

        /**
            @brief Pops @p n elements from the stack.
         */
        auto pop_n (size_type const n) -> void;

        /**
            @brief Erases all elements from the container.
         */
        auto clear () -> void;

    private:
        auto range_check (size_type const offset) const -> void;
    };

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack() :
        base {}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(Container const& cont) :
        base {cont}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(Container&& cont) :
        base {std::move(cont)}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(peekable_stack const& other) :
        base {other}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(peekable_stack&& other) :
        base {std::move(other)}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(Alloc const& alloc) :
        base {alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(Container const& cont, Alloc const& alloc) :
        base {cont, alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(Container&& cont, Alloc const& alloc) :
        base {std::move(cont), alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(peekable_stack const& other, Alloc const& alloc) :
        base {other, alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(peekable_stack&& other, Alloc const& alloc) :
        base {std::move(other), alloc}
    {
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::operator[]
        (size_type const offset) -> reference
    {
        return const_cast<reference>(const_cast<peekable_stack const&>(*this).operator[](offset));
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::operator[]
        (size_type const offset) const -> const_reference
    {
        auto it = base::c.rbegin(); // TODO fucking no... add peek and peek_safe()
        std::advance(it, offset);
        return *it;
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::peek
        (size_type const offset) -> reference
    {
        return const_cast<reference>(const_cast<peekable_stack const&>(*this).peek(offset));
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::peek
        (size_type const offset) const -> const_reference
    {
        this->range_check(offset);
        auto it = base::c.rbegin();
        std::advance(it, offset);
        return *it;
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::pop_n
        (size_type const n) -> void
    {
        for (auto i = 0u; i < n; ++i)
        {
            this->pop();
        }
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::clear
        () -> void
    {
        base::c.clear();
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::range_check
        (size_type const offset) const -> void
    {
        if (offset >= base::size())
        {
            throw std::out_of_range {"Peek offset (" + std::to_string(offset) + ") out of range."};
        }
    }
}

#endif