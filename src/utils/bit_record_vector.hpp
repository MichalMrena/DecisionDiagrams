#ifndef MIX_UTILS_BIT_RECORD_VECTOR
#define MIX_UTILS_BIT_RECORD_VECTOR

#include <vector>
#include <cstdint>

namespace mix::utils
{
    template<uint8_t RecordBitCount>
    class proxy_setter
    {
    public:
        friend class bit_record_vector<RecordBitCount>;

    private:
        const int32_t* dataPtr;
        const size_t  recordIndex;

    public:
        /*
            Sets value of the record at associated index.
            @return value of the data argument
        */
        auto operator= (const int8_t data) -> int8_t;

    private:
        proxy_setter(const int32_t* pDataPtr
                   , const size_t  pRecordIndex);
    };

    template<uint8_t RecordBitCount>
    class bit_record_vector
    {
    private:
        std::vector<int8_t> dataVector;

    public:
        bit_record_vector();
        ~bit_record_vector();

        auto operator[] (const size_t i) const -> int8_t;
        auto operator[] (const size_t i)       -> proxy_setter<RecordBitCount>;

        auto get (const size_t i) const -> int8_t;
        auto set (const size_t i
                , const int8_t val)     -> void;
    };
    
}

#endif