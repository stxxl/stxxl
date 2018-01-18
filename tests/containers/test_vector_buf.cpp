/***************************************************************************
 *  tests/containers/test_vector_buf.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <algorithm>
#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/scan>
#include <stxxl/stream>
#include <stxxl/vector>

struct my_type  // 24 bytes, not a power of 2 intentionally
{
    uint64_t key;
    uint64_t load0;
    uint64_t load1;

    explicit my_type(uint64_t i = 0)
        : key(i),
          load0(i + 1),
          load1(1 + 42)
    { }

    bool operator == (const my_type& b) const
    {
        return (key == b.key) && (load0 == b.load0) && (load1 == b.load1);
    }
};

//! Verify contents of the vector
template <typename VectorType>
void check_vector(const VectorType& v)
{
    using value_type = typename VectorType::value_type;

    for (size_t i = 0; i < v.size(); ++i)
    {
        die_unless(v[i] == value_type(i));
    }
}

//! Stream object generating lots of ValueTypes
template <typename ValueType>
class MyStream
{
    uint64_t i;

public:
    using value_type = ValueType;

    MyStream()
        : i(0)
    { }

    value_type operator * () const
    {
        return value_type(i);
    }

    MyStream& operator ++ ()
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
void test_vector_buf(size_t size)
{
    using vector_type = typename stxxl::vector<ValueType>;

    {   // fill vector using element access
        foxxll::scoped_print_timer tm("element access");

        vector_type vec(size);

        for (size_t i = 0; i < size; ++i)
            vec[i] = ValueType(i);

        check_vector(vec);
    }
    {   // fill vector using iterator access
        foxxll::scoped_print_timer tm("iterator access");

        vector_type vec(size);

        auto vi = vec.begin();

        for (size_t i = 0; i < size; ++i, ++vi)
            *vi = ValueType(i);

        check_vector(vec);
    }
    {   // fill vector using vector_bufwriter
        foxxll::scoped_print_timer tm("vector_bufwriter");

        vector_type vec(size);

        typename vector_type::bufwriter_type writer(vec.begin());

        for (size_t i = 0; i < size; ++i)
            writer << ValueType(i);

        writer.finish();

        check_vector(vec);
    }
    {   // fill empty vector using vector_bufwriter
        foxxll::scoped_print_timer tm("empty vector_bufwriter");

        vector_type vec;

        typename vector_type::bufwriter_type writer(vec);

        for (size_t i = 0; i < size; ++i)
            writer << ValueType(i);

        writer.finish();

        check_vector(vec);
    }

    vector_type vec(size);

    {   // fill vector using materialize
        foxxll::scoped_print_timer tm("materialize");

        MyStream<ValueType> stream;
        stxxl::stream::materialize(stream, vec.begin(), vec.end());

        check_vector(vec);
    }
    {   // read vector using vector_bufreader
        foxxll::scoped_print_timer tm("vector_bufreader");

        const vector_type& cvec = vec;

        typename vector_type::bufreader_type reader(cvec.begin(), cvec.end());

        for (size_t i = 0; i < size; ++i)
        {
            die_unless(!reader.empty());
            die_unless(reader.size() == size - i);

            ValueType pv = *reader;

            ValueType v;
            reader >> v;

            die_unless(v == ValueType(i));
            die_unless(pv == v);
            die_unless(reader.size() == size - i - 1);
        }

        die_unless(reader.empty());

        // rewind reader and read again
        reader.rewind();

        for (size_t i = 0; i < size; ++i)
        {
            die_unless(!reader.empty());
            die_unless(reader.size() == size - i);

            ValueType pv = *reader;

            ValueType v;
            reader >> v;

            die_unless(v == ValueType(i));
            die_unless(pv == v);
            die_unless(reader.size() == size - i - 1);
        }

        die_unless(reader.empty());
    }
    {   // read vector using vector_bufreader_reverse
        foxxll::scoped_print_timer tm("vector_bufreader_reverse");

        const vector_type& cvec = vec;

        typename vector_type::bufreader_reverse_type reader(cvec.begin(), cvec.end());

        for (size_t i = 0; i < size; ++i)
        {
            die_unless(!reader.empty());
            die_unless(reader.size() == size - i);

            ValueType pv = *reader;

            ValueType v;
            reader >> v;

            die_unless(v == ValueType(size - i - 1));
            die_unless(pv == v);
            die_unless(reader.size() == size - i - 1);
        }

        die_unless(reader.empty());

        // rewind reader and read again
        reader.rewind();

        for (size_t i = 0; i < size; ++i)
        {
            die_unless(!reader.empty());
            die_unless(reader.size() == size - i);

            ValueType pv = *reader;

            ValueType v;
            reader >> v;

            die_unless(v == ValueType(size - i - 1));
            die_unless(pv == v);
            die_unless(reader.size() == size - i - 1);
        }

        die_unless(reader.empty());
    }
    {   // read vector using C++11 for loop construct
        foxxll::scoped_print_timer tm("C++11 for loop");

        uint64_t i = 0;

        for (auto it : vec)
        {
            die_unless(it == ValueType(i));
            ++i;
        }

        die_unless(i == vec.size());
    }
    {   // read vector using C++11 for loop construct
        foxxll::scoped_print_timer tm("C++11 bufreader for loop");
        using bufreader_type = typename vector_type::bufreader_type;

        uint64_t i = 0;

        for (auto it : bufreader_type(vec))
        {
            die_unless(it == ValueType(i));
            ++i;
        }

        die_unless(i == vec.size());
    }
}

int main(int argc, char* argv[])
{
    int size = (argc > 1) ? atoi(argv[1]) : 1;

    LOG1 << "Testing stxxl::vector<int> with even size";
    test_vector_buf<int>(size * STXXL_DEFAULT_BLOCK_SIZE(int) / 64);

    LOG1 << "Testing stxxl::vector<int> with odd size";
    test_vector_buf<int>(size * STXXL_DEFAULT_BLOCK_SIZE(int) / 64 + 501 + 42);

    LOG1 << "Testing stxxl::vector<uint64_t>";
    test_vector_buf<uint64_t>(size * STXXL_DEFAULT_BLOCK_SIZE(uint64_t) / 64 + 501 + 42);

    LOG1 << "Testing stxxl::vector<my_type>";
    test_vector_buf<my_type>(size * STXXL_DEFAULT_BLOCK_SIZE(my_type) / 64);

    return 0;
}
