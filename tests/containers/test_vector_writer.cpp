/***************************************************************************
 *  containers/test_vector_writer.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <algorithm>
#include <stxxl/vector>
#include <stxxl/scan>
#include <stxxl/stream>

using stxxl::uint64;

struct my_type  // 24 bytes, not a power of 2 intentionally
{
    uint64 key;
    uint64 load0;
    uint64 load1;

    my_type(uint64 i = 0)
        : key(i),
          load0(i + 1),
          load1(1 + 42)
    {
    }

    bool operator == (const my_type& b) const
    {
        return (key == b.key) && (load0 == b.load0) && (load1 == b.load1);
    }
};

//! Verify contents of the vector
template <typename VectorType>
void check_vector(const VectorType& v)
{
    typedef typename VectorType::value_type value_type;

    for (uint64 i = 0; i < v.size(); ++i)
    {
        STXXL_CHECK( v[i] == value_type(i) );
    }
}

//! Stream object generating lots of ValueTypes
template <typename ValueType>
class MyStream
{
    uint64      i;

public:
    typedef ValueType value_type;

    MyStream() :
        i(0)
    {}

    value_type operator*() const
    {
        return value_type(i);
    }

    MyStream& operator++()
    {
        ++i;
        return *this;
    }

    bool empty() const
    {
        return false;
    }
};

template <typename ValueType>
void test_vector_writer(uint64 size)
{
    typedef typename stxxl::VECTOR_GENERATOR<ValueType>::result vector_type;

    { // fill vector using element access
        stxxl::scoped_print_timer tm("element access");

        vector_type v(size);

        for (uint64 i = 0; i < size; ++i)
            v[i] = ValueType(i);

        check_vector(v);
    }
    { // fill vector using iterator access
        stxxl::scoped_print_timer tm("iterator access");

        vector_type v(size);

        typename vector_type::iterator vi = v.begin();

        for (uint64 i = 0; i < size; ++i, ++vi)
            *vi = ValueType(i);

        check_vector(v);
    }
    { // fill vector using vector_bufwriter
        stxxl::scoped_print_timer tm("vector_bufwriter");

        vector_type v(size);

        stxxl::vector_bufwriter<vector_type> writer(v.begin());

        for (uint64 i = 0; i < size; ++i)
            writer << ValueType(i);

        writer.finish();

        check_vector(v);
    }
    { // fill vector using materialize

        stxxl::scoped_print_timer tm("materialize");

        vector_type v(size);

        MyStream<ValueType> stream;
        stxxl::stream::materialize(stream, v.begin(), v.end());

        check_vector(v);
    }
}

int main()
{
    STXXL_MSG("Testing stxxl::vector<int>");
    test_vector_writer<int>(64 * 1024*1024);

    STXXL_MSG("Testing stxxl::vector<uint64>");
    test_vector_writer<uint64>(64 * 1024*1024);

    STXXL_MSG("Testing stxxl::vector<my_type>");
    test_vector_writer<my_type>(32 * 1024*1024);

    return 0;
}
