/*
 * Copyright(C):    WhiZTiM, 2015
 *
 * This file is part of the TIML::UBEX C++14 library
 *
 * Distributed under the Boost Software License, Version 1.0.
 *      (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author: Ibrahim Timothy Onogu
 * Email:  ionogu@acm.org
 */

#ifndef STREAM_WRITER_HPP
#define STREAM_WRITER_HPP

#include <fstream>
#include <algorithm>
#include "value.hpp"
#include "stream_helpers.hpp"

namespace timl {

    std::pair<Type, bool> common_array_type(const Value&);

    template<typename StreamType>
    class StreamWriter
    {
    public:
        StreamWriter(StreamType& Stream);

        std::pair<std::size_t, bool> writeValue(const Value&);
        StreamType& getStream() { return stream; }

    private:

        std::pair<size_t, bool> append_key(const std::string&);

        std::pair<size_t, bool> append_object(const Value&);
        std::pair<size_t, bool> append_value(const Value&);
        std::pair<size_t, bool> append_null();
        std::pair<size_t, bool> append_char(char);
        std::pair<size_t, bool> append_bool(bool);
        std::pair<size_t, bool> append_float(double);
        std::pair<size_t, bool> append_signedInt(long long);
        std::pair<size_t, bool> append_unsignedInt(unsigned long long, bool evaluate_uint64 = true);
        std::pair<size_t, bool> append_size(std::size_t);

        std::pair<size_t, bool> append_string(const std::string&);
        std::pair<size_t, bool> append_binary(const Value::BinaryType&);
        std::pair<size_t, bool> append_array(const Value&);

        void update(const std::pair<size_t, bool>&, std::pair<size_t, bool>&);

        bool write(Marker);
        bool write(byte);
        bool write(const byte *, std::size_t);

        StreamType& stream;
    };


    template<typename StreamType>
    StreamWriter<StreamType>::StreamWriter(StreamType& Stream)
        : stream(Stream) {}

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::writeValue(const Value& value)
    {
        if(not value.isMap())
            return std::make_pair(0, false);
        return append_object(value);
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_object(const Value& value)
    {
        auto keys = value.keys();
        std::pair<size_t, bool> rtn(1, false);
        write(Marker::Object_Start);
        update(append_size(keys.size()), rtn);

        for(const auto& key : keys)
        {
            decltype(rtn) k(0, false);
            k = append_key(key);
            update(k, rtn);
            k = append_value(value[key]);
            update(k, rtn);
        }
        write(Marker::Object_End);
        rtn.first += 1;
        return rtn;
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_value(const Value& v)
    {
        std::pair<size_t, bool> k(0, false);
        if(v.isNull())
            k = append_null();
        else if(v.isBool())
            k = append_bool(v);
        else if(v.isChar())
            k = append_char(v);
        else if(v.isSignedInteger())
            k = append_signedInt(v);
        else if(v.isUnsignedInteger())
            k = append_unsignedInt(v);
        else if(v.isFloat())
            k = append_float(v);
        else if(v.isString())
            k = append_string(v);
        else if(v.isBinary())
            k = append_binary(v);
        else if(v.isArray())
            k = append_array(v);
        else if(v.isObject())
            k = append_object(v);
        return k;
    }

    template<typename StreamType>
    bool StreamWriter<StreamType>::write(Marker m)
    {
        return write(static_cast<byte>(m));
    }

    template<typename StreamType>
    bool StreamWriter<StreamType>::write(byte b)
    {
        return write(&b, 1);
    }

    template<typename StreamType>
    bool StreamWriter<StreamType>::write(const byte* b, std::size_t sz)
    {
        stream.write(reinterpret_cast<const char*>(b), sz);
        return true;
    }

    template<typename StreamType>
    inline void StreamWriter<StreamType>::update(const std::pair<size_t, bool>& src, std::pair<size_t, bool>& dest)
    {
        dest.first += src.first;
        dest.second = src.second;
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_null()
    {
        write(static_cast<byte>(Marker::Null));
        return std::make_pair(1, true);
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_bool(bool b)
    {
        if(b)
            write(static_cast<byte>(Marker::True));
        else
            write(static_cast<byte>(Marker::False));
        return std::make_pair(1, true);
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_char(char c)
    {
        write(static_cast<byte>(Marker::Char));
        write(static_cast<byte>(c));
        return std::make_pair(2, true);
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_key(const std::string& key)
    {
        //! \todo Assert key.size() is less than 256 characters
        if(key.size() > 255)
            throw std::logic_error("Don't you obey invariants?");

        std::size_t key_size = key.size();
        write(static_cast<byte>(key_size));
        write(reinterpret_cast<const byte*>(key.c_str()), key_size);

        return std::make_pair(1 + key_size, true);
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_binary(const Value::BinaryType& bin)
    {
        write(Marker::Binary);
        auto rtn = append_size(bin.size());
        write(bin.data(), bin.size());
        rtn.first += bin.size() + 1;
        return rtn;
    }


    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_string(const std::string& str)
    {
        const std::size_t size = str.size();
        write(Marker::String);
        auto rtn = append_size(size);
        write(reinterpret_cast<const byte*>(str.data()), size);
        rtn.first += size + 1;
        return rtn;
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_size(std::size_t sz)
    {
        return append_unsignedInt(sz, false);
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_unsignedInt(unsigned long long val, bool evaluate_uint64)
    {
        using Uint8 = std::numeric_limits<uint8_t>;
        using Uint16 = std::numeric_limits<uint16_t>;
        using Uint32 = std::numeric_limits<uint32_t>;
        using Uint64 = std::numeric_limits<uint64_t>;

        byte b[8];
        std::pair<size_t, bool> rtn(0, false);
        if(in_range(val, Uint8::lowest(), Uint8::max()))
        {
            write(Marker::Uint8);
            write(static_cast<byte>(val));
            rtn = std::make_pair(2, true);
        }
        else if(in_range(val, Uint16::lowest(), Uint16::max()))
        {
            const uint16_t ct = static_cast<uint16_t>(val);
            const uint16_t val = toBigEndian16(ct);
            std::memcpy(b, &val, 2);
            write(Marker::Uint16);
            write(b, 2);
            rtn = std::make_pair(3, true);
        }
        else if(in_range(val, Uint32::lowest(), Uint32::max()))
        {
            const uint32_t ct = static_cast<uint32_t>(val);
            const uint32_t val = toBigEndian32(ct);
            std::memcpy(b, &val, 4);
            write(Marker::Uint32);
            write(b, 4);
            rtn = std::make_pair(5, true);
        }
        else if(evaluate_uint64 and in_range(val, Uint64::lowest(), Uint64::max()))
        {
            const uint64_t ct = static_cast<uint64_t>(val);
            const uint64_t val = toBigEndian64(ct);
            std::memcpy(b, &val, 8);
            write(Marker::Uint64);
            write(b, 8);
            rtn = std::make_pair(9, true);
        }

        return rtn;
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_signedInt(long long val)
    {
        using Int8 = std::numeric_limits<int8_t>;
        using Int16 = std::numeric_limits<int16_t>;
        using Int32 = std::numeric_limits<int32_t>;
        using Int64 = std::numeric_limits<int64_t>;

        //An optimization... val could be a positive integer
        //  if so, chances are it may be more efficeint to append as unsignedInt()..
        //Example: if val = 226
        //  this function will require int16_t to represent, while
        //  its sister function (unsigned) can fit it into int8_t
        if(val >= 0)
            return append_unsignedInt(val);
        //End Optimization


        byte b[8];
        std::pair<size_t, bool> rtn(0, false);
        if(in_range(val, Int8::lowest(), Int8::max()))
        {
            write(Marker::Int8);
            write(static_cast<byte>(val));
            rtn = std::make_pair(2, true);
        }
        else if(in_range(val, Int16::lowest(), Int16::max()))
        {
            const uint16_t tit = static_cast<uint16_t>(val);
            const uint16_t val = toBigEndian16(tit);
            std::memcpy(b, &val, 2);
            write(Marker::Int16);
            write(b, 2);
            rtn = std::make_pair(3, true);
        }
        else if(in_range(val, Int32::lowest(), Int32::max()))
        {
            const uint32_t tit = static_cast<uint32_t>(val);
            const uint32_t val = toBigEndian32(tit);
            std::memcpy(b, &val, 4);
            write(Marker::Int32);
            write(b, 4);
            rtn = std::make_pair(5, true);
        }
        else if(in_range(val, Int64::lowest(), Int64::max()))
        {
            const uint64_t tit = static_cast<uint64_t>(val);
            const uint64_t val = toBigEndian64(tit);
            std::memcpy(b, &val, 8);
            write(Marker::Int64);
            write(b, 8);
            rtn = std::make_pair(9, true);
        }

        return rtn;
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_float(double val)
    {
        using Float32 = std::numeric_limits<float>;
        using Float64 = std::numeric_limits<double>;

        byte b[8];
        std::pair<size_t, bool> rtn(0, false);
        if(in_range(val, Float32::lowest(), Float32::max()) and static_cast<float>(val) == val)
        {
            const uint32_t val_i = toBigEndianFloat32(static_cast<float>(val));
            std::memcpy(b, &val_i, 4);
            write(Marker::Float32);
            write(b, 4);
            rtn = std::make_pair(5, true);
        }
        else if(in_range(val, Float64::lowest(), Float64::max()))
        {
            const uint64_t val_i = toBigEndianFloat64(val);
            std::memcpy(b, &val_i, 8);
            write(Marker::Float64);
            write(b, 8);
            rtn = std::make_pair(9, true);
        }

        return rtn;
    }

    template<typename StreamType>
    std::pair<size_t, bool> StreamWriter<StreamType>::append_array(const Value& value)
    {
        std::pair<size_t, bool> rtn(2, false);
        const std::size_t size = value.size();
        write(Marker::HetroArray_Start);

        if(size != 0)           //empty array optimization... dont append size if zero
            update(append_size(size), rtn);

        for(size_t i=0; i < size; i++)
            update(append_value(value[i]), rtn);

        write(Marker::HetroArray_End);
        return rtn;

        //! \todo TODO... detect homogenepus arrays and treat accordingly
        //! WORK in progress

    }











    std::pair<Type, bool> common_array_type(const Value& value)
    {
        std::pair<Type, bool> rtn(Type::Null, false);

        if( not value.isArray() or value.size() < 2)
            return rtn;

        const auto type1 = value[0].type();    //pick the type of the first element
        for(size_t i = 1; i < value.size(); ++i)
        {
            const auto type2 = value[i].type();
            if(type1 != type2)
                return rtn;
        }
        rtn.first = type1;
        rtn.second = isDirectType(type1) or isSequenceType(type1) ? true : false;
        return rtn;
    }







}   //end namespace timl

#endif // STREAM_WRITER_HPP

