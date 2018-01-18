/***************************************************************************
 *  examples/containers/vector1.cpp
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
#include <iostream>

#include <stxxl/vector>

int main()
{
    stxxl::vector<int> my_vector;

    for (int i = 0; i < 1024 * 1024; i++)
    {
        my_vector.push_back(i + 2);
    }

    std::cout << my_vector[99] << std::endl;
    my_vector[100] = 0;

    while (!my_vector.empty())
    {
        my_vector.pop_back();
    }

    return 0;
}
//! [example]
