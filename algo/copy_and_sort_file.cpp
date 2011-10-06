/***************************************************************************
 *  algo/copy_and_sort_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
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

#include <limits>
#include <stxxl/io>
#include <stxxl/vector>
#include <stxxl/stream>


struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    char _data[128 - sizeof(key_type)];
    key_type key() const
    {
        return _key;
    }

    my_type() { }
    my_type(key_type __key) : _key(__key) { }

    static my_type min_value()
    {
        return my_type((std::numeric_limits<key_type>::min)());
    }
    static my_type max_value()
    {
        return my_type((std::numeric_limits<key_type>::max)());
    }
};


inline bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}

inline bool operator == (const my_type & a, const my_type & b)
{
    return a.key() == b.key();
}

struct Cmp
{
    typedef my_type first_argument_type;
    typedef my_type second_argument_type;
    typedef bool result_type;
    bool operator () (const my_type & a, const my_type & b) const
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

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj._key;
    return o;
}


int main(int argc, char ** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " infile outfile" << std::endl;
        return -1;
    }

    const unsigned memory_to_use = 512 * 1024 * 1024;
    const unsigned int block_size = sizeof(my_type) * 4096;

    typedef stxxl::vector<my_type, 1, stxxl::lru_pager<2>, block_size> vector_type;

    stxxl::syscall_file in_file(argv[1], stxxl::file::DIRECT | stxxl::file::RDONLY);
    stxxl::syscall_file out_file(argv[2], stxxl::file::DIRECT | stxxl::file::RDWR | stxxl::file::CREAT);
    vector_type input(&in_file);
    vector_type output(&out_file);
    output.resize(input.size());

#ifdef BOOST_MSVC
    typedef stxxl::stream::streamify_traits<vector_type::iterator>::stream_type input_stream_type;
#else
    typedef __typeof__(stxxl::stream::streamify(input.begin(), input.end())) input_stream_type;
#endif
    input_stream_type input_stream = stxxl::stream::streamify(input.begin(), input.end());

    typedef Cmp comparator_type;
    typedef stxxl::stream::sort<input_stream_type, comparator_type, block_size> sort_stream_type;
    sort_stream_type sort_stream(input_stream, comparator_type(), memory_to_use);

    vector_type::iterator o = stxxl::stream::materialize(sort_stream, output.begin(), output.end());
    assert(o == output.end());

    if (1) {
        STXXL_MSG("Checking order...");
        STXXL_MSG((stxxl::is_sorted(output.begin(), output.end(), comparator_type()) ? "OK" : "WRONG"));
    }

    return 0;
}

// vim: et:ts=4:sw=4
