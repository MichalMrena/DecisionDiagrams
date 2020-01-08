#ifndef _MIX_DD_BIT_VECTOR_
#define _MIX_DD_BIT_VECTOR_

#include <type_traits>
#include <vector>
#include <cstdint>
#include <cmath>
#include "../utils/math_utils.hpp"

namespace mix::dd
{
    template<size_t RecordBitSize, class ValueType = int8_t, class Enable = void>
    class bit_vector;

    template<size_t RecordBitSize>
    using EnableIfPolicy = typename std::enable_if<RecordBitSize <= 32>::type;

    template<size_t RecordBitSize, class ValueType>
    class proxy_ref
    {
    public:
        using vec_t = bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>;

    private:
        vec_t& vec;
        size_t recordIndex;
        
    public:
        proxy_ref(const size_t pRecordIndex, vec_t& pVec);

        auto operator= (const ValueType val) -> proxy_ref&;
        operator ValueType () const;
    };

    template<size_t RecordBitSize, class ValueType>
    class bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>
    {
    public:
        using proxy_ref_t = proxy_ref<RecordBitSize, ValueType>;

    private:
        using is_two_pow = std::integral_constant<bool, utils::is_power_of_two<RecordBitSize>::value>;

        std::vector<int64_t> blocks;
        size_t recordCount;

    public:
        ~bit_vector() = default;

        bit_vector();
        bit_vector(const size_t initialSize);

        auto get (const size_t i) const  -> ValueType;
        auto add (const ValueType val)   -> void;
        auto set (const size_t i
                , const ValueType val)   -> void;
        auto operator[] (const size_t i) -> proxy_ref_t;

    private:
        auto get (const size_t i, std::true_type)  const -> ValueType;
        auto get (const size_t i, std::false_type) const -> ValueType;

        auto set (const size_t i, const ValueType val, std::true_type)  -> void;
        auto set (const size_t i, const ValueType val, std::false_type) -> void;
    };  

    template<size_t RecordBitSize, class ValueType>
    bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::bit_vector
        () :
        recordCount {0}
    {
    }

    template<size_t RecordBitSize, class ValueType>
    bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::bit_vector
        (const size_t initialSize) :
        recordCount {0}
    {
        this->blocks.reserve(
            static_cast<size_t>(std::ceil((initialSize * RecordBitSize) / sizeof(int64_t)))
        );
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::get
        (const size_t i) const -> ValueType
    {
        return this->get(i, is_two_pow {});
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::add
        (const ValueType val) -> void
    {
        if (this->recordCount * RecordBitSize >= sizeof(int64_t) * this->blocks.size())
        {
            this->blocks.push_back(0);
        }

        this->set(this->recordCount++, val, is_two_pow {});
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::set
        (const size_t i, const ValueType val) -> void
    {
        this->set(i, val, is_two_pow {});
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::operator[]
        (const size_t i) -> proxy_ref_t
    {
        return proxy_ref_t {i, *this};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::get
        (const size_t i, std::true_type) const -> ValueType
    {
        const size_t  recordsInBlock {(sizeof(int64_t) * 8) / RecordBitSize};
        const size_t  blockIndex     {i / recordsInBlock};
        const size_t  recordOffset   {i % recordsInBlock};
        const int64_t mask           {RecordBitSize | (RecordBitSize - 1)};

        return static_cast<ValueType>(
            (this->blocks[blockIndex] >> (recordOffset * RecordBitSize)) & mask
        );
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::get
        (const size_t i, std::false_type) const -> ValueType
    {
        throw std::runtime_error {"Not supported yet."};
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::set
        (const size_t i, const ValueType val, std::true_type)  -> void
    {
        const size_t  recordsInBlock {(sizeof(int64_t) << 3) / RecordBitSize};
        const size_t  blockIndex     {i / recordsInBlock};
        const size_t  recordOffset   {i % recordsInBlock};
        const int64_t mask           {RecordBitSize | (RecordBitSize - 1)};
        const int64_t valMask        {val & mask};

        this->blocks[blockIndex] &= ~(mask << recordOffset * RecordBitSize);        
        this->blocks[blockIndex] |= valMask << recordOffset * RecordBitSize;
    }

    template<size_t RecordBitSize, class ValueType>
    auto bit_vector<RecordBitSize, ValueType, EnableIfPolicy<RecordBitSize>>::set
        (const size_t i, const ValueType val, std::false_type)  -> void
    {
        throw std::runtime_error {"Not supported yet."};
    }

    template<size_t RecordBitSize, class ValueType>
    proxy_ref<RecordBitSize, ValueType>::proxy_ref
        (const size_t pRecordIndex, vec_t& pVec) :
        vec {pVec}
      , recordIndex {pRecordIndex}
    {
    }

    template<size_t RecordBitSize, class ValueType>
    auto proxy_ref<RecordBitSize, ValueType>::operator=
        (const ValueType val) -> proxy_ref&
    {
        this->vec.set(this->recordIndex, val);
        return *this;
    }

    template<size_t RecordBitSize, class ValueType>
    proxy_ref<RecordBitSize, ValueType>::operator ValueType
        () const
    {
        return this->vec.get(this->recordIndex);
    }
}

#endif