#ifndef MIX_DD_LIST_MAP_
#define MIX_DD_LIST_MAP_

#include <vector>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <memory>
#include <type_traits>

namespace mix::ds
{
    namespace aux_impl
    {
        template<class T, class = std::void_t<>>
        struct is_transparent : public std::false_type { };
        
        template<class T>
        struct is_transparent<T, std::void_t<typename T::is_transparent>> : public std::true_type { };

        template<class T>
        inline constexpr auto is_transparent_v = is_transparent<T>::value; 
    }

    template< class Key
            , class T
            , class KeyEqual  = std::equal_to<Key>
            , class Container = std::vector<std::pair<Key const, T>> >
    class simple_map
    {
    public:
        using container_type  = Container;
        using value_type      = std::pair<Key const, T>;
        using key_type        = Key;
        using reference       = T&;
        using const_reference = T const&;
        using iterator        = typename container_type::iterator;
        using const_iterator  = typename container_type::const_iterator;
        using size_type       = std::size_t;

        template<class Alloc>
        using uses_allocator  = typename std::enable_if_t<std::uses_allocator_v<container_type, Alloc>>;

    public:
        simple_map () = default;
        simple_map (std::initializer_list<value_type> init);
        simple_map (simple_map const& other)     noexcept(std::is_nothrow_copy_constructible_v<Container>);
        simple_map (simple_map&& other)          noexcept(std::is_nothrow_move_constructible_v<Container>);
        explicit simple_map (Container const& c) noexcept(std::is_nothrow_copy_constructible_v<Container>);
        explicit simple_map (Container&& c)      noexcept(std::is_nothrow_move_constructible_v<Container>);

        template<class Alloc, class = uses_allocator<Alloc>> explicit simple_map (Alloc const&);
        template<class Alloc, class = uses_allocator<Alloc>> simple_map (Container const&,  Alloc const&);
        template<class Alloc, class = uses_allocator<Alloc>> simple_map (Container&&,       Alloc const&);
        template<class Alloc, class = uses_allocator<Alloc>> simple_map (simple_map const&, Alloc const&);
        template<class Alloc, class = uses_allocator<Alloc>> simple_map (simple_map&&,      Alloc const&);

        auto at         (Key const& k)                            -> reference;
        auto at         (Key const& k) const                      -> const_reference;
        auto find       (Key const& k)                            -> iterator;
        auto find       (Key const& k) const                      -> const_iterator;
        auto operator[] (Key const& k)                            -> reference;
        auto operator[] (Key&& k)                                 -> reference;
        auto insert     (value_type const& v)                     -> std::pair<iterator, bool>;
        auto insert     (value_type&& v)                          -> std::pair<iterator, bool>;
        auto insert     (std::initializer_list<value_type> ilist) -> void;
        auto contains   (Key const& k ) const                     -> bool;
        auto erase      (key_type const& k)                       -> size_type;
        auto erase      (iterator pos)                            -> iterator;
        auto erase      (const_iterator pos)                      -> iterator;
        
        template<class K>       auto find        (K const& k )                       -> std::enable_if_t<aux_impl::is_transparent_v<K>, iterator>;
        template<class K>       auto find        (K const& k ) const                 -> std::enable_if_t<aux_impl::is_transparent_v<K>, const_iterator>;
        template<class K>       auto contains    (K const& k ) const                 -> std::enable_if_t<aux_impl::is_transparent_v<K>, bool>;
        template<class... Args> auto emplace     (Args&&... args)                    -> std::pair<iterator, bool>;
        template<class... Args> auto try_emplace (key_type const& k, Args&&... args) -> std::pair<iterator, bool>;
        template<class... Args> auto try_emplace (key_type&& k, Args&&... args)      -> std::pair<iterator, bool>;
        template<class P>       auto insert      (P&& v)                             -> std::enable_if_t<std::is_constructible_v<value_type, P&&>, std::pair<iterator,bool>>;
        template<class InputIt> auto insert      (InputIt first, InputIt last)       -> void;
        
        auto operator= (simple_map rhs) -> simple_map&;
        auto clear     ()               -> void;
        auto begin     ()               -> iterator;
        auto end       ()               -> iterator;
        auto size      () const         -> size_type;
        auto max_size  () const         -> size_type;
        auto begin     () const         -> const_iterator;
        auto end       () const         -> const_iterator;
        auto cbegin    () const         -> const_iterator;
        auto cend      () const         -> const_iterator;
        auto swap      (simple_map& rhs) noexcept(std::is_nothrow_swappable_v<Container>) -> void;

    private:
        template<class K, class... Args> 
        auto try_emplace_impl (K&& k, Args&&... args) -> std::pair<iterator, bool>;
        
        template<class V>       
        auto insert_impl (V&& v) -> std::pair<iterator, bool>;
        
        template<class K>       
        auto bracket_op_impl (K&& k) -> reference;
        
        template<class K, class Inserter, class... Args > 
        auto possibly_insert (K&& k, Inserter&& inserter, Args&&... args) -> std::pair<iterator, bool>; 

        template<class K>
        auto find_impl (K const& k) const -> const_iterator;

        auto to_iterator (const_iterator cit) -> iterator;
        auto it_to_last  ()                   -> iterator;
        auto it_to_last  () const             -> const_iterator;

    private:
        container_type data_;
    };

    template<class Key, class T, class KeyEqual = std::equal_to<T>>
    auto make_simple_map (std::size_t const initialSize = 4) -> simple_map<Key, T, KeyEqual, std::vector<std::pair<const Key, T>>>;

    template<class Key, class T, class KeyEqual, class Container>
    auto operator== ( simple_map<Key, T, KeyEqual, Container> const& lhs
                    , simple_map<Key, T, KeyEqual, Container> const& rhs ) -> bool;

    template<class Key, class T, class KeyEqual, class Container>
    auto operator!= ( simple_map<Key, T, KeyEqual, Container> const& lhs
                    , simple_map<Key, T, KeyEqual, Container> const& rhs ) -> bool;

    template<class Key, class T, class KeyEqual, class Container>
    auto swap ( simple_map<Key, T, KeyEqual, Container> const& lhs
              , simple_map<Key, T, KeyEqual, Container> const& rhs ) -> void;

// implementation:
    
    template<class Key, class T, class KeyEqual, class Container>
    simple_map<Key, T, KeyEqual, Container>::simple_map
        (Container const& c) noexcept(std::is_nothrow_copy_constructible_v<Container>):
        data_ {c}
    {
    }
    
    template<class Key, class T, class KeyEqual, class Container>
    simple_map<Key, T, KeyEqual, Container>::simple_map
        (Container&& c) noexcept(std::is_nothrow_move_constructible_v<Container>) :
        data_ {std::move(c)}
    {
    }
    
    template<class Key, class T, class KeyEqual, class Container>
    simple_map<Key, T, KeyEqual, Container>::simple_map
        (simple_map const& other) noexcept(std::is_nothrow_copy_constructible_v<Container>):
        data_ {other.data}
    {
    }
    
    template<class Key, class T, class KeyEqual, class Container>
    simple_map<Key, T, KeyEqual, Container>::simple_map
        (simple_map&& other) noexcept(std::is_nothrow_move_constructible_v<Container>) :
        data_ {std::move(other.data_)}
    {
    }
    
    template<class Key, class T, class KeyEqual, class Container>
    simple_map<Key, T, KeyEqual, Container>::simple_map
        (std::initializer_list<value_type> init) :
        data_ {init}
    {
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class Alloc, class>
    simple_map<Key, T, KeyEqual, Container>::simple_map(Alloc const& alloc) :
        data_ {alloc}
    {
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class Alloc, class>
    simple_map<Key, T, KeyEqual, Container>::simple_map(Container const& cont, Alloc const& alloc) :
        data_ {cont, alloc}
    {
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class Alloc, class>
    simple_map<Key, T, KeyEqual, Container>::simple_map(Container&& cont, Alloc const& alloc) :
        data_ {std::move(cont), alloc}
    {
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class Alloc, class>
    simple_map<Key, T, KeyEqual, Container>::simple_map(simple_map const& other, Alloc const& alloc) :
        data_ {other, alloc}
    {
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class Alloc, class>
    simple_map<Key, T, KeyEqual, Container>::simple_map(simple_map&& other, Alloc const& alloc) :
        data_ {std::move(other), alloc}
    {
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::at
        (Key const& k) -> reference
    {
        return const_cast<reference>(const_cast<simple_map const*>(this)->at(k));
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::at
        (Key const& k) const -> const_reference
    {
        auto const it = this->find(k);

        if (this->end() == it)
        {
            throw std::out_of_range {"Key not found."};
        }

        return it->second;
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::find
        (Key const& k) -> iterator
    {
        return this->to_iterator(const_cast<simple_map const*>(this)->find(k));
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::find
        (Key const& k) const -> const_iterator
    {
        return this->find_impl(k);
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::operator[]
        (Key const& k) -> reference
    {
        return this->bracket_op_impl(k);
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::operator[]
        (Key&& k) -> reference
    {
        return this->bracket_op_impl(std::move(k));
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::operator=
        (simple_map rhs) -> simple_map&
    {
        rhs.swap(*this);
        return *this;
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::clear
        () -> void
    {
        data_.clear();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::size
        () const -> size_type
    {
        return data_.size();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::max_size
        () const -> size_type
    {
        return data_.max_size();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::swap
        (simple_map& rhs) noexcept(std::is_nothrow_swappable_v<Container>) -> void
    {
        using std::swap;
        swap(data_, rhs.data_);
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::begin
        () -> iterator
    {
        return this->data_.begin();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::end
        () -> iterator
    {
        return this->data_.end();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::begin
        () const -> const_iterator
    {
        return this->data_.cbegin();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::end
        () const -> const_iterator
    {
        return this->data_.cend();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::cbegin
        () const -> const_iterator
    {
        return this->begin();
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::cend
        () const -> const_iterator
    {
        return this->end();
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class... Args>
    auto simple_map<Key, T, KeyEqual, Container>::emplace
        (Args&&... args) -> std::pair<iterator, bool>
    {
        auto pair      = value_type {std::forward<Args>(args)...};
        auto const k   = pair.first;
        auto const ins = [this](auto&&, auto&& p) { this->data_.emplace_back(std::move(p)); };
        return this->possibly_insert(k, ins, std::move(pair));
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class... Args>
    auto simple_map<Key, T, KeyEqual, Container>::try_emplace
        (key_type const& k, Args&&... args) -> std::pair<iterator, bool>
    {
        return this->try_emplace_impl(k, std::forward<Args>(args)...);
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class... Args>
    auto simple_map<Key, T, KeyEqual, Container>::try_emplace
        (key_type&& k, Args&&... args) -> std::pair<iterator, bool>
    {
        return this->try_emplace_impl(std::move(k), std::forward<Args>(args)...);
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class P>
    auto simple_map<Key, T, KeyEqual, Container>::insert
        (P&& v) -> std::enable_if_t<std::is_constructible_v<value_type, P&&>, std::pair<iterator,bool>>
    {
        return this->emplace(std::forward<P>(v));
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::insert
        (value_type const& v) -> std::pair<iterator, bool>
    {
        return this->insert_impl(v);
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::insert
        (value_type&& v) -> std::pair<iterator, bool>
    {
        return this->insert_impl(std::move(v));
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class InputIt>
    auto simple_map<Key, T, KeyEqual, Container>::insert
        (InputIt first, InputIt last) -> void
    {
        while (first != last)
        {
            this->insert(*first++);
        }
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::insert
        (std::initializer_list<value_type> ilist) -> void
    {
        this->insert(std::begin(ilist), std::end(ilist));
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::contains
        (Key const& k) const -> bool
    {
        return this->end() == this->find(k);
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::erase
        (key_type const& k) -> size_type
    {
        auto const it = this->find(k);
        if (this->end() == it)
        {
            return 0;
        }

        this->erase(it);
        return 1;
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::erase
        (iterator pos) -> iterator
    {
        if (this->it_to_last() != pos)
        {
            auto const pErased = std::addressof(*pos);
            auto alloc = data_.get_allocator();
            using traits_t = std::allocator_traits<decltype(alloc)>;
            traits_t::destroy(alloc, pErased);
            traits_t::construct(alloc, pErased, std::move(data_.back()));
            data_.pop_back();
        }

        return std::next(pos);
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::erase
        (const_iterator pos) -> iterator
    {
        return this->erase(this->to_iterator(pos));
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class K, class... Args>
    auto simple_map<Key, T, KeyEqual, Container>::try_emplace_impl
        (K&& k, Args&&... args) -> std::pair<iterator, bool>
    {
        auto const ins = [this](auto&& k, auto&&... args) 
        { data_.emplace_back( std::piecewise_construct
                            , std::forward_as_tuple(std::forward<K>(k))
                            , std::forward_as_tuple(std::forward<Args>(args)...) ); };
        return this->possibly_insert(std::forward<K>(k), ins, std::forward<Args>(args)...);
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class V>
    auto simple_map<Key, T, KeyEqual, Container>::insert_impl
        (V&& v) -> std::pair<iterator, bool>
    {
        auto const ins = [this](auto&&, auto&& v) 
        { this->data_.emplace_back(std::forward<V>(v)); };
        return this->possibly_insert(v.first, ins, std::forward<V>(v));
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class K>
    auto simple_map<Key, T, KeyEqual, Container>::bracket_op_impl
        (K&& k) -> reference
    {
        auto const ins = [this](auto&& k) 
        { data_.emplace_back( std::piecewise_construct
                            , std::forward_as_tuple(std::forward<K>(k))
                            , std::tuple<> {} ); };
        return this->possibly_insert(std::forward<K>(k), ins).first->second;
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::to_iterator
        (const_iterator cit) -> iterator
    {
        auto it = this->begin();
        std::advance(it, std::distance(this->cbegin(), cit));
        return it;
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::it_to_last
        () -> iterator
    {
        return std::prev(this->end());
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto simple_map<Key, T, KeyEqual, Container>::it_to_last
        () const -> const_iterator
    {
        return std::prev(this->end());
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class K, class Inserter, class... Args>
    auto simple_map<Key, T, KeyEqual, Container>::possibly_insert
        (K&& k, Inserter&& inserter, Args&&... args) -> std::pair<iterator, bool>
    {
        auto const it = this->find(k);
        if (this->end() != it)
        {
            return std::make_pair(it, false);
        }

        inserter(std::forward<K>(k), std::forward<Args>(args)...);
        
        return std::make_pair(this->it_to_last(), true);
    }

    template<class Key, class T, class KeyEqual, class Container>
    template<class K>
    auto simple_map<Key, T, KeyEqual, Container>::find_impl
        (K const& k) const -> const_iterator
    {
        return std::find_if( std::begin(data_), std::end(data_)
                           , [&k, eq = KeyEqual {}](auto const& p) { return eq(p.first, k); } );
    }


    template<class Key, class T, class KeyEqual>
    auto make_simple_map (std::size_t const initialSize) -> simple_map<Key, T, KeyEqual, std::vector<std::pair<const Key, T>>>
    {
        auto vec = std::vector<std::pair<const Key, T>> {};
        vec.reserve(initialSize);
        return simple_map<Key, T, KeyEqual, decltype(vec)> {std::move(vec)};
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto operator== ( simple_map<Key, T, KeyEqual, Container> const& lhs
                    , simple_map<Key, T, KeyEqual, Container> const& rhs ) -> bool
    {
        return lhs.size() == rhs.size() 
            && std::is_permutation(std::begin(lhs), std::end(lhs), std::begin(rhs));
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto operator!= ( simple_map<Key, T, KeyEqual, Container> const& lhs
                    , simple_map<Key, T, KeyEqual, Container> const& rhs ) -> bool
    {
        return ! (lhs == rhs);
    }

    template<class Key, class T, class KeyEqual, class Container>
    auto swap ( simple_map<Key, T, KeyEqual, Container> const& lhs
              , simple_map<Key, T, KeyEqual, Container> const& rhs ) -> void
    {
        return lhs.swap(rhs);
    }
}

#endif