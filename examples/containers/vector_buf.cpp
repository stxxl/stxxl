/***************************************************************************
 *  examples/containers/vector_buf.cpp
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
#include <stxxl/vector>
#include <stxxl/bits/config.h>

using stxxl::uint64;

void test_vector_iterator(uint64 size)
{
    stxxl::scoped_print_timer tm("vector iterator access");

//! [iterator]
    typedef stxxl::VECTOR_GENERATOR<uint64>::result vector_type;

    vector_type vec(size);

    for (uint64 i = 0; i < vec.size(); ++i)
        vec[i] = i;

    uint64 sum = 0;
    for (uint64 i = 0; i < vec.size(); ++i)
        sum += vec[i];
//! [iterator]

    std::cout << "sum: " << sum << std::endl;
    STXXL_CHECK( sum == size * (size-1) / 2 );
}

void test_vector_buffered(uint64 size)
{
    stxxl::scoped_print_timer tm("vector buffered access");

//! [buffered]
    typedef stxxl::VECTOR_GENERATOR<uint64>::result vector_type;

    vector_type vec(size);

    // write using vector_bufwriter
    vector_type::bufwriter_type writer(vec);

    for (uint64 i = 0; i < vec.size(); ++i)
        writer << i;

    // required to flush out the last block (or destruct the bufwriter)
    writer.finish();

    // now read using vector_bufreader
    uint64 sum = 0;

    for (vector_type::bufreader_type reader(vec); !reader.empty(); ++reader)
    {
        sum += *reader;
    }
//! [buffered]

    std::cout << "sum: " << sum << std::endl;
    STXXL_CHECK( sum == size * (size-1) / 2 );
}

#if (STXXL_MSVC && STXXL_HAVE_CXX11) || (defined(__GNUG__) && (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40600)
void test_vector_cxx11(uint64 size)
{
    stxxl::scoped_print_timer tm("vector C++11 loop access");

    typedef stxxl::VECTOR_GENERATOR<uint64>::result vector_type;

    vector_type vec(size);

    {
        vector_type::bufwriter_type writer(vec);

        for (uint64 i = 0; i < vec.size(); ++i)
            writer << i;
    }

//! [cxx11]
    // now using vector_bufreader adaptor to C++11 for loop
    uint64 sum = 0;

    for (auto it : vector_type::bufreader_type(vec))
    {
        sum += it;
    }
//! [cxx11]

    std::cout << "sum: " << sum << std::endl;
    STXXL_CHECK( sum == size * (size-1) / 2 );
}
#endif

int main()
{
    const size_t size = 128 * 1024*1024;

    stxxl::block_manager::get_instance();

    test_vector_iterator(size);
    test_vector_buffered(size);

#if (STXXL_MSVC && STXXL_HAVE_CXX11) || (defined(__GNUG__) && (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40600)
    test_vector_cxx11(size);
#endif

    return 0;
}
