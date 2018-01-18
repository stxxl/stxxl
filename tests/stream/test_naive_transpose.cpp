/***************************************************************************
 *  tests/stream/test_naive_transpose.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

//! \example stream/test_naive_transpose.cpp
//! This is an example of how to use some basic algorithms from the
//! stream package. The example transposes a 2D-matrix which is serialized
//! as an 1D-vector.
//!
//! This transpose implementation is trivial, not neccessarily fast or
//! efficient. There are more sophisticated and faster algorithms for external
//! memory matrix transpostion than sorting, see e.g. J.S. Vitter: Algorithms
//! and Data Structures for External Memory, Chapter 7.2.

#include <iostream>
#include <limits>
#include <vector>

#include <tlx/die.hpp>

#include <stxxl/bits/defines.h>
#include <stxxl/stream>
#include <stxxl/vector>

class streamop_matrix_transpose
{
    unsigned cut, repeat;
    unsigned pos;

public:
    using value_type = unsigned;

    streamop_matrix_transpose(unsigned cut, unsigned repeat) : cut(cut), repeat(repeat), pos(0)
    { }

    value_type operator * () const
    {
        return (pos % cut) * repeat + pos / cut;
    }

    streamop_matrix_transpose& operator ++ ()
    {
        ++pos;
        return *this;
    }

    bool empty() const
    {
        return pos == (cut * repeat);
    }
};

template <typename T>
struct cmp_tuple_first : std::binary_function<T, T, bool>
{
    using value_type = T;
    using first_value_type = typename value_type::first_type;

    bool operator () (const value_type& a, const value_type& b) const
    {
        return a.first < b.first;
    }

    value_type min_value() const
    {
        return value_type(std::numeric_limits<first_value_type>::min(), 0);
    }

    value_type max_value() const
    {
        return value_type(std::numeric_limits<first_value_type>::max(), 0);
    }
};

template <typename Vector>
void dump_upper_left(const Vector& v, unsigned rows, unsigned cols, unsigned nx, unsigned ny)
{
    int w = 5;

    // assumes row-major layout in the vector serialization of the matrix
    for (unsigned y = 0; y < ny && y < rows; ++y) {
        std::cout << std::setw(w) << y << ":";
        for (unsigned x = 0; x < nx && x < cols; ++x)
            std::cout << " " << std::setw(w) << v[y * cols + x];
        if (nx < cols)
            std::cout << " ...";
        std::cout << std::endl;
    }
    if (ny < rows)
        std::cout << std::setw(w) << "..." << std::endl;
    std::cout << std::endl;
}

int main()
{
    unsigned num_cols = 1000;
    unsigned num_rows = 500;

    // buffers for streamify and materialize,
    // block size matches the block size of the input/output vector
    size_t numbuffers = 2 * foxxll::config::get_instance()->disks_number();

    // RAM to be used for sorting (in bytes)
    size_t memory_for_sorting = 1 << 28;

    ///////////////////////////////////////////////////////////////////////

    using array_type = stxxl::vector<unsigned>;

    array_type input(num_rows* num_cols);
    array_type output(num_cols* num_rows);

    // fill the input array with some values
    for (unsigned i = 0; i < num_rows * num_cols; ++i)
        input[i] = i;

    std::cout << "Before transpose:" << std::endl;
    dump_upper_left(input, num_rows, num_cols, 10, 10);

    foxxll::stats_data stats_before(*foxxll::stats::get_instance());

    // HERE streaming part begins (streamifying)
    // create input stream
    using input_stream_type = stxxl::stream::streamify_traits<array_type::iterator>::stream_type;
    input_stream_type input_stream = stxxl::stream::streamify(input.begin(), input.end(), numbuffers);

    // create stream of destination indices
    using destination_index_stream_type = streamop_matrix_transpose;
    destination_index_stream_type destination_index_stream(num_cols, num_rows);

    // create tuple stream: (key, value)
    using tuple_stream_type = stxxl::stream::make_tuple<destination_index_stream_type, input_stream_type>;
    tuple_stream_type tuple_stream(destination_index_stream, input_stream);

    // sort tuples by first entry (key)
    using cmp_type = cmp_tuple_first<tuple_stream_type::value_type>;
    using sorted_tuple_stream_type = stxxl::stream::sort<tuple_stream_type, cmp_type>;
    sorted_tuple_stream_type sorted_tuple_stream(tuple_stream, cmp_type(), memory_for_sorting);

    // discard the key we used for sorting, keep second entry of the tuple only (value)
    using sorted_element_stream_type = stxxl::stream::choose<sorted_tuple_stream_type, 2>;
    sorted_element_stream_type sorted_element_stream(sorted_tuple_stream);

    // HERE streaming part ends (materializing)
    array_type::iterator o = stxxl::stream::materialize(sorted_element_stream, output.begin(), output.end(), numbuffers);
    die_unless(o == output.end());
    die_unless(sorted_element_stream.empty());

    foxxll::stats_data stats_after(*foxxll::stats::get_instance());

    std::cout << "After transpose:" << std::endl;
    dump_upper_left(output, num_cols, num_rows, 10, 10);

    std::cout << "I/O stats (streaming part only!)" << std::endl << (stats_after - stats_before);

    return 0;
}

// vim: et:ts=4:sw=4
