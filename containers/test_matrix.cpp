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

struct constant_one
{
	const constant_one& operator++() const { return *this; }
	bool empty() const { return false; }
	int operator*() const { return 1; }
};

int main()
{
    const int rank = 1024;
    stxxl::matrix<double, 1024> A(rank, rank), B(rank, rank), C(rank, rank);

    const stxxl::unsigned_type internal_memory = 1ull * 1024 * 1024 * 1024;
    
    clock_t start, end;
    
    constant_one co;
    A.materialize_from_row_major(co, internal_memory);
    B.materialize_from_row_major(co, internal_memory);
    C.materialize_from_row_major(co, internal_memory);
    
    STXXL_MSG("Multiplying two " << rank << "x" << rank << " matrices.");

    stxxl::stats_data stats_before(*stxxl::stats::get_instance());

    stxxl::multiply(A, B, C, internal_memory);
    
    stxxl::stats_data stats_after(*stxxl::stats::get_instance());

    std::cout << (stats_after - stats_before);

    return 0;
}
