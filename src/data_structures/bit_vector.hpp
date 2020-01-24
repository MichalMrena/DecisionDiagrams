#ifndef _MIX_DD_BIT_VECTOR_
#define _MIX_DD_BIT_VECTOR_

#include <type_traits>
#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include "../utils/math_utils.hpp"

namespace mix::dd
{
    template<size_t RecordBitSize, class ValueType>
    class bit_vector;

    template<size_t RecordBitSize, class ValueType>
    auto swap ( bit_vector<RecordBitSize, ValueType>& lhs
              , bit_vector<RecordBitSize, ValueType>& rhs ) noexcept -> void;

    /**
        Proxy ref.
    */
    template<size_t RecordBitSize, class ValueType>
    class proxy_ref
    {
    public:
        using vec_t = bit_vector<RecordBitSize, ValueType>;

    private:
        vec_t& vec;
        size_t recordIndex;
        
    public:
        proxy_ref(const size_t pRecordIndex, vec_t& pVec);

        auto operator= (const ValueType val) -> proxy_ref&;
        operator ValueType () const;
    };

    /**
        Iterator.
    */
    template<size_t RecordBitSize, class ValueType>
    class bit_v_iterator
    {
    private:
        using bit_v_t = bit_vector<RecordBitSize, ValueType>;

    private:
        const bit_vector<RecordBitSize, ValueType>& vector;
        size_t currentPos;

    public:
        bit_v_iterator (const bit_v_iterator&) = default;
        bit_v_iterator (bit_v_iterator&&)      = default;
        
        bit_v_iterator ( const bit_v_t& vector
                       , const size_t initialPos );

        auto operator!= (const bit_v_iterator& other) const -> bool;
        auto operator== (const bit_v_iterator& other) const -> bool;
        auto operator-  (const bit_v_iterator& other) const -> size_t;
        auto operator+  (const size_t i)              const -> bit_v_iterator;
        auto operator*  () const -> ValueType;
        auto operator++ ()       -> bit_v_iterator&;
    };

    /**
        Bit vector.
    */
    template<size_t RecordBitSize, class ValueType>
    class bit_vector
    {
        static_assert(RecordBitSize <= 32, "Bit size of a record must be less than 32.");
        static_assert(RecordBitSize <= sizeof(ValueType) << 3, "Value type must fit into given bit count.");
        static_assert(std::is_integral_v<ValueType>, "ValueType must be of integral type.");

    public:
        friend class proxy_ref<RecordBitSize, ValueType>;
        friend auto swap<RecordBitSize, ValueType> 
                        ( bit_vector<RecordBitSize, ValueType>& lhs
                        , bit_vector<RecordBitSize, ValueType>& rhs ) noexcept -> void;

    private:
        using is_two_pow   = std::integral_constant<bool, utils::is_power_of_two(RecordBitSize)>;
        using case_two_pow = std::true_type;
        using case_general = std::false_type;

        #if _WIN64 || __amd64__
            using word_t = uint64_t;
        #else
            using word_t = uint32_t;
        #endif

    public:
        using proxy_ref_t    = proxy_ref<RecordBitSize, ValueType>;
        using size_type      = std::vector<word_t>::size_type;
        using iterator       = bit_v_iterator<RecordBitSize, ValueType>;
        using const_iterator = bit_v_iterator<RecordBitSize, ValueType>;

    private:
        size_t recordCount;
        std::vector<word_t> blocks;

    public:
        bit_vector ();
        bit_vector (const size_t initialSize);
        bit_vector (const bit_vector& other);
        bit_vector (bit_vector&& other);

        auto at         (const size_t i) const -> ValueType;
        auto push_back  (const ValueType val)  -> void;
        auto operator[] (const size_t i)       -> proxy_ref_t;
        auto size       () const               -> size_type;
        auto begin      ()                     -> iterator;
        auto end        ()                     -> iterator;
        auto begin      () const               -> const_iterator;
        auto end        () const               -> const_iterator;

    private:
        auto get (const size_t i, case_two_pow) const -> ValueType;
        auto get (const size_t i, case_general) const -> ValueType;

        auto set (const size_t i, const ValueType val, case_two_pow) -> void;
        auto set (const size_t i, const ValueType val, case_general) -> void;

        auto ensure_capacity () -> void;
    };

    template<size_t RecordBitSize, class ValueType>
    auto swap ( bit_vector<RecordBitSize, ValueType>& lhs
              , bit_vector<RecordBitSize, ValueType>& rhs ) noexcept -> void
    {
        using std::swap;
        swap(lhs.blocks, rhs.blocks);
    }

    template<size_t RecordBitSize, class ValueType>
    bit_vector<RecordBitSize, ValueType>::bit_vector
        () :
        recordCount {0}
    {
    }

    template<size_t RecordBitSize, class ValueType>
    bit_vector<RecordBitSize, ValueType>::bit_vector
        (const size_t initialSize) :
        recordCount {0}
    {
        this->blocks.reserve(
            static_cast<size_t>(
                std::ceil((initialSize * RecordBitSize) / static_cast<double>(sizeof(word_t)))
            )
        );
    }

    template<size_t RecordBitSize, class ValueType>
    bit_vector<RecordBitSize, ValueType>::bit_vector
        (const bit_vector& other) :
        recordCount {other.recordCount}
      , blocks      {other.blocks}
    {
    }

    template<size_t RecordBitSize, class ValueType>
    bit_vector<RecordBitSize, ValueType>::bit_vector
        (bit_vector&& other) :
        recordCount {other.recordCount}
      , blocks      {std::move(other.blocks)}
    {
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::at
        (const size_t i) const -> ValueType
    {
        return this->get(i, is_two_pow {});
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::push_back
        (const ValueType val) -> void
    {
        this->ensure_capacity();
        this->set(this->recordCount++, val, is_two_pow {});
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::operator[]
        (const size_t i) -> proxy_ref_t
    {
        return proxy_ref_t {i, *this};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::size
        () const -> size_type
    {
        return this->recordCount;
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::begin
        () -> iterator
    {
        return iterator {*this, 0};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::end
        () -> iterator
    {
        return iterator {*this, this->recordCount};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::begin
        () const -> const_iterator
    {
        return const_iterator {*this, 0};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::end
        () const -> const_iterator
    {
        return const_iterator {*this, this->recordCount};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::get
        (const size_t i, case_two_pow) const -> ValueType
    {
        const size_t  recordsInBlock {(sizeof(word_t) << 3) / RecordBitSize};
        const size_t  blockIndex     {i / recordsInBlock};
        const size_t  recordOffset   {i % recordsInBlock};
        const word_t  mask           {RecordBitSize | (RecordBitSize - 1)};

        return static_cast<ValueType>(
            (this->blocks[blockIndex] >> (recordOffset * RecordBitSize)) & mask
        );
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::get
        (const size_t i, case_general) const -> ValueType
    {
        throw std::runtime_error {"Not supported yet."};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::set
        (const size_t i, const ValueType val, case_two_pow)  -> void
    {
        const size_t recordsInBlock {(sizeof(word_t) << 3) / RecordBitSize};
        const size_t blockIndex     {i / recordsInBlock};
        const size_t recordOffset   {i % recordsInBlock};
        const word_t mask           {RecordBitSize | (RecordBitSize - 1)};
        const word_t valMask        {val & mask};

        this->blocks[blockIndex] &= ~(mask << recordOffset * RecordBitSize);        
        this->blocks[blockIndex] |= valMask << recordOffset * RecordBitSize;
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::set
        (const size_t i, const ValueType val, case_general)  -> void
    {
        throw std::runtime_error {"Not supported yet."};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType>::ensure_capacity
        () -> void
    {
        const auto bitsAvaliable {(sizeof(word_t) << 3) * this->blocks.size()};
        const auto bitsNeeded    {(1 + this->recordCount) * RecordBitSize};

        if (bitsNeeded > bitsAvaliable)
        {
            this->blocks.push_back(0);
        }
    }

    // bit_v_iterator:

    template<size_t RecordBitSize, class ValueType>
    bit_v_iterator<RecordBitSize, ValueType>::bit_v_iterator
        ( const bit_v_t& pVector
        , const size_t pInitialPos ) :
        vector {pVector}
      , currentPos {pInitialPos}
    {
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_v_iterator<RecordBitSize, ValueType>::operator!=
        (const bit_v_iterator& other) const -> bool
    {
        return this->currentPos != other.currentPos;
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_v_iterator<RecordBitSize, ValueType>::operator==
        (const bit_v_iterator& other) const -> bool
    {
        return ! (*this != other);
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_v_iterator<RecordBitSize, ValueType>::operator-
        (const bit_v_iterator& other) const -> size_t
    {
        return this->currentPos - other.currentPos;
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_v_iterator<RecordBitSize, ValueType>::operator+
        (const size_t i) const -> bit_v_iterator
    {
        return bit_v_iterator {this->vector, this->currentPos + i};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_v_iterator<RecordBitSize, ValueType>::operator*
        () const -> ValueType
    {
        return this->vector.at(this->currentPos);
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_v_iterator<RecordBitSize, ValueType>::operator++
        () -> bit_v_iterator&
    {
        ++this->currentPos;
        return *this;
    }

    // proxy_ref:

    template<size_t RecordBitSize, class ValueType>
    proxy_ref<RecordBitSize, ValueType>::proxy_ref
        (const size_t pRecordIndex, vec_t& pVec) :
        vec         {pVec}
      , recordIndex {pRecordIndex}
    {
    }

    template<size_t RecordBitSize, class ValueType>
    auto proxy_ref<RecordBitSize, ValueType>::operator=
        (const ValueType val) -> proxy_ref&
    {
        using itp = typename vec_t::is_two_pow;
        this->vec.set(this->recordIndex, val, itp {});
        return *this;
    }

    template<size_t RecordBitSize, class ValueType>
    proxy_ref<RecordBitSize, ValueType>::operator ValueType
        () const
    {
        return this->vec.at(this->recordIndex);
    }
}

#endif