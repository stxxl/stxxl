/***************************************************************************
 *  include/stxxl/bits/containers/test_matrix.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/containers/matrix.h>
#include <stxxl/vector>
#include <stxxl/stream>

int main()
{
    const int rank = 10000;
    stxxl::matrix<double> A(rank, rank), B(rank, rank), C(rank, rank);

    typedef stxxl::VECTOR_GENERATOR<double>::result input_type;
    input_type input(rank * rank);

#ifdef BOOST_MSVC
    typedef stxxl::stream::streamify_traits<input_type::iterator>::stream_type input_stream_type;
#else
    typedef __typeof__(stxxl::stream::streamify(input.begin(), input.end())) input_stream_type;
#endif
    input_stream_type input_stream = stxxl::stream::streamify(input.begin(), input.end());

    A.materialize_from_row_major(input_stream);

    const stxxl::unsigned_type internal_memory = 1ull * 1024 * 1024 * 1024;
    stxxl::multiply(A, B, C, internal_memory);

    return 0;
}
