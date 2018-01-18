/***************************************************************************
 *  examples/algo/copy_and_sort_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example algo/copy_and_sort_file.cpp
//! This example imports a file into an \c stxxl::vector without copying its
//! content and then sorts it using \c stxxl::stream::sort while writing the
//! sorted output to a different file.

#include <tlx/logger.hpp>

#include <foxxll/io.hpp>

#include <stxxl/stream>
#include <stxxl/vector>

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

struct my_type
{
    using key_type = unsigned;

    key_type m_key;
    char m_data[128 - sizeof(key_type)];

    my_type() { }
    explicit my_type(key_type k) : m_key(k) { }

    static my_type min_value()
    {
        return my_type(std::numeric_limits<key_type>::min());
    }
    static my_type max_value()
    {
        return my_type(std::numeric_limits<key_type>::max());
    }
};

inline bool operator < (const my_type& a, const my_type& b)
{
    return a.m_key < b.m_key;
}

inline bool operator == (const my_type& a, const my_type& b)
{
    return a.m_key == b.m_key;
}

struct Cmp
{
    using first_argument_type = my_type;
    using second_argument_type = my_type;
    using result_type = bool;
    bool operator () (const my_type& a, const my_type& b) const
    {
        return a < b;
    }
    static my_type min_value()
    {
        return my_type::min_value();
    }
    static my_type max_value()
    {
        return my_type::max_value();
    }
};

std::ostream& operator << (std::ostream& o, const my_type& obj)
{
    o << obj.m_key;
    return o;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " infile outfile" << std::endl;
        return -1;
    }

    const size_t memory_to_use = 512 * 1024 * 1024;
    const size_t block_size = sizeof(my_type) * 4096;

    using vector_type = stxxl::vector<my_type, 1, stxxl::lru_pager<2>, block_size>;

    foxxll::file_ptr in_file = tlx::make_counting<foxxll::syscall_file>(
        argv[1], foxxll::file::DIRECT | foxxll::file::RDONLY);
    foxxll::file_ptr out_file = tlx::make_counting<foxxll::syscall_file>(
        argv[2], foxxll::file::DIRECT | foxxll::file::RDWR | foxxll::file::CREAT);
    vector_type input(in_file);
    vector_type output(out_file);
    output.resize(input.size());

    using input_stream_type = stxxl::stream::streamify_traits<vector_type::iterator>::stream_type;
    input_stream_type input_stream = stxxl::stream::streamify(input.begin(), input.end());

    using comparator_type = Cmp;
    using sort_stream_type = stxxl::stream::sort<input_stream_type, comparator_type, block_size>;
    sort_stream_type sort_stream(input_stream, comparator_type(), memory_to_use);

    vector_type::iterator o = stxxl::stream::materialize(sort_stream, output.begin(), output.end());
    assert(o == output.end());
    tlx::unused(o);

    if (1) {
        LOG1 << "Checking order...";
        LOG1 << (stxxl::is_sorted(output.cbegin(), output.cend(), comparator_type()) ? "OK" : "WRONG");
    }

    return 0;
}

// vim: et:ts=4:sw=4
