#ifndef MIX_DS_PEEKABLE_STACK_
#define MIX_DS_PEEKABLE_STACK_

#include <deque>
#include <stack>
#include <stdexcept>

namespace mix::ds
{
    /**
     *  They say that one does not simply inherit from std:: Container.
     *  However I've decided that this is the simplest approach to achieve desired result.
     *  I don't see any situations when using this container would result in something bad.
     *  If you see a problem with it please let me know.
     * 
     *  @brief Extension of std::stack that allows you to peek elements under top() using peek(offset) method. 
     */
    template<class T, class Container = std::deque<T>>
    class peekable_stack : public std::stack<T, Container>
    {
    public:
        using base_t          = std::stack<T, Container>;
        using container_type  = typename base_t::container_type;
        using value_type      = typename base_t::size_type;
        using reference       = typename base_t::reference;
        using const_reference = typename base_t::const_reference;
        using size_type       = typename base_t::size_type;

        template<class Alloc>
        using uses_allocator  = typename std::enable_if_t<std::uses_allocator<container_type, Alloc>::value>;

    public: // See documentation for std::stack constructors.
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

        /**
         *  @return reference to the element that is @p offset positions under the top(). 
         *  @throw  std::out_of_range
         */
        auto peek (size_type const offset) -> reference;

        /**
         *  @return const_reference to the element that is @p offset positions under the top(). 
         *  @throw  std::out_of_range
         */
        auto peek (size_type const offset) const -> const_reference;

        auto clear () -> void;

    private:
        auto range_check (size_type const offset) const -> void;
    };    

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack() :
        base_t {}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(Container const& cont) :
        base_t {cont}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(Container&& cont) :
        base_t {std::move(cont)}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(peekable_stack const& other) :
        base_t {other}
    {
    }

    template<class T, class Container>
    peekable_stack<T, Container>::peekable_stack(peekable_stack&& other) :
        base_t {std::move(other)}
    {
    }
    
    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(Alloc const& alloc) :
        base_t {alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(Container const& cont, Alloc const& alloc) :
        base_t {cont, alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(Container&& cont, Alloc const& alloc) :
        base_t {std::move(cont), alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(peekable_stack const& other, Alloc const& alloc) :
        base_t {other, alloc}
    {
    }

    template<class T, class Container>
    template<class Alloc, class>
    peekable_stack<T, Container>::peekable_stack(peekable_stack&& other, Alloc const& alloc) :
        base_t {std::move(other), alloc}
    {
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
        auto it = base_t::c.rbegin();
        std::advance(it, offset);
        return *it;
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::clear
        () -> void
    {
        base_t::c.clear();
    }

    template<class T, class Container>
    auto peekable_stack<T, Container>::range_check
        (size_type const offset) const -> void
    {
        if (offset >= base_t::size())
        {
            throw std::out_of_range {"Peek offset (" + std::to_string(offset) + ") out of range."};
        }
    }
}

#endif  