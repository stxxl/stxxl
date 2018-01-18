/***************************************************************************
 *  examples/containers/vector_buf.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>

#include <tlx/die.hpp>

#include <stxxl/bits/config.h>
#include <stxxl/vector>

void test_vector_element(uint64_t size)
{
    foxxll::scoped_print_timer tm("vector element access", 2 * size * sizeof(uint64_t));

//! [element]
    stxxl::vector<uint64_t> vec(size);

    for (uint64_t i = 0; i < vec.size(); ++i)
        vec[i] = (i % 1024);

    uint64_t sum = 0;
    for (uint64_t i = 0; i < vec.size(); ++i)
        sum += vec[i];
//! [element]

    std::cout << "sum: " << sum << std::endl;
    die_unless(sum == size / 1024 * (1024 * 1023 / 2));
}

void test_vector_iterator(uint64_t size)
{
    foxxll::scoped_print_timer tm("vector iterator access", 2 * size * sizeof(uint64_t));

//! [iterator]
    stxxl::vector<uint64_t> vec(size);

    uint64_t i = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it, ++i)
        *it = (i % 1024);

    uint64_t sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it)
        sum += *it;
//! [iterator]

    std::cout << "sum: " << sum << std::endl;
    die_unless(sum == size / 1024 * (1024 * 1023 / 2));
}

void test_vector_buffered(uint64_t size)
{
    foxxll::scoped_print_timer tm("vector buffered access", 2 * size * sizeof(uint64_t));

//! [buffered]
    using vector_type = stxxl::vector<uint64_t>;

    vector_type vec(size);

    // write using vector_bufwriter
    vector_type::bufwriter_type writer(vec);

    for (uint64_t i = 0; i < vec.size(); ++i)
        writer << (i % 1024);

    // required to flush out the last block (or destruct the bufwriter)
    writer.finish();

    // now read using vector_bufreader
    uint64_t sum = 0;

    for (vector_type::bufreader_type reader(vec); !reader.empty(); ++reader)
    {
        sum += *reader;
    }
//! [buffered]

    std::cout << "sum: " << sum << std::endl;
    die_unless(sum == size / 1024 * (1024 * 1023 / 2));
}

void test_vector_cxx11(uint64_t size)
{
    foxxll::scoped_print_timer tm("vector C++11 loop access", 2 * size * sizeof(uint64_t));

    using vector_type = stxxl::vector<uint64_t>;

    vector_type vec(size);

    {
        vector_type::bufwriter_type writer(vec);

        for (uint64_t i = 0; i < vec.size(); ++i)
            writer << (i % 1024);
    }

//! [cxx11]
    // now using vector_bufreader adaptor to C++11 for loop
    uint64_t sum = 0;

    for (auto it : vector_type::bufreader_type(vec))
    {
        sum += it;
    }
//! [cxx11]

    std::cout << "sum: " << sum << std::endl;
    die_unless(sum == size / 1024 * (1024 * 1023 / 2));
}

int main(int argc, char* argv[])
{
    int multi = (argc >= 2 ? atoi(argv[1]) : 64);
    const uint64_t size = multi * 1024 * uint64_t(1024) / sizeof(uint64_t);

    foxxll::block_manager::get_instance();

    test_vector_element(size);
    test_vector_iterator(size);
    test_vector_buffered(size);

    test_vector_cxx11(size);

    return 0;
}
