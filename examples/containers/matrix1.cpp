/***************************************************************************
 *  examples/containers/matrix1.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Daniel Feist <daniel.feist@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! [example]
#include <stxxl/bits/containers/matrix.h>

#include <iostream>

int main()
{
    // Matrix dimensions
    int height = 3;
    int width = 3;

    int internal_memory = 64 * 1024 * 1024;
    const int small_block_order = 32;  // must be multiple of matrix valueType in bits

    using block_schedular_type = stxxl::block_scheduler<stxxl::matrix_swappable_block<int, small_block_order> >;
    using matrix_type = stxxl::matrix<int, small_block_order>;

    block_schedular_type my_bs(internal_memory);

    // Create 3 matrices with given dimensions
    matrix_type A(my_bs, height, width);
    matrix_type B(my_bs, height, width);
    matrix_type C(my_bs, height, width);

    using row_iterator = matrix_type::row_major_iterator;

    int i = 0;

    // Fill matrix A with values 0,1,2,3,...
    for (row_iterator it_A = A.begin(); it_A != A.end(); ++it_A, ++i)
    {
        *it_A = i;
    }

    i = 0;

    // Fill matrix B with values 0,2,4,8,...
    for (row_iterator it_B = B.begin(); it_B != B.end(); ++it_B, ++i)
    {
        *it_B = i * 2;
    }

    // Multiply matrix A and B and store result in matrix C
    C = A * B;

    C.transpose();

    // Print out matrix C
    for (row_iterator it_C = C.begin(); it_C != C.end(); ++it_C)
    {
        std::cout << *it_C << " ";
    }

    return 0;
}
//! [example]
