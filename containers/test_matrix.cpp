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

#include <time.h>
#include <iostream>

int main()
{
    const int rank = 1000;
    stxxl::matrix<double> A(rank, rank), B(rank, rank), C(rank, rank);

    typedef stxxl::VECTOR_GENERATOR<double>::result input_type;
    input_type inputA(rank * rank), inputB(rank * rank), inputC(rank * rank);

#ifdef BOOST_MSVC
    typedef stxxl::stream::streamify_traits<input_type::iterator>::stream_type input_stream_type;
#else
    typedef __typeof__(stxxl::stream::streamify(inputA.begin(), inputA.end())) input_stream_type;
#endif
    input_stream_type input_streamA = stxxl::stream::streamify(inputA.begin(), inputA.end()),
            input_streamB = stxxl::stream::streamify(inputB.begin(), inputB.end()),
            input_streamC = stxxl::stream::streamify(inputC.begin(), inputC.end());

    const stxxl::unsigned_type internal_memory = 1ull * 1024 * 1024 * 1024;
    
    clock_t start, end;
    
    start = clock();
    
    A.materialize_from_row_major(input_streamA, internal_memory);
    B.materialize_from_row_major(input_streamB, internal_memory);
    C.materialize_from_row_major(input_streamC, internal_memory);
    
    end = clock();
    std::cout << "Time required for materialize_from_row_major: " << (double)(end-start)/CLOCKS_PER_SEC 
            << " seconds." << "\n\n";
    
    start = clock();
    
    stxxl::multiply(A, B, C, internal_memory);
    
    end = clock();
    std::cout << "Time required for multiply: " << (double)(end-start)/CLOCKS_PER_SEC 
            << " seconds." << "\n\n";

    return 0;
}
