/***************************************************************************
 *  examples/containers/vector1.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
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
    // <value_type, page_size, number_of_pages, block_size, alloc_strategy, paging_strategy> 
    typedef stxxl::VECTOR_GENERATOR<int, 4, 8, 1*1024*1024, stxxl::RC, stxxl::lru>::result vector_type;
  
    vector_type V;
    int counter = 0;

    srand(903428594);

    // fill vector with random integers

    for (size_t i = 0; i < 10000000; ++i) {
        V.push_back(rand() % 10000);  
    }

    std::cout << "Size of vector V is: " << V.size() << std::endl;
  
    // iterate over vector 

    vector_type::const_iterator iter = V.begin();
 
    for (size_t j = 0; j < V.size(); ++j) {
        //std::cout << *iter << " ";     
        if (*iter % 2 == 0) {  // is V's current element even?
            ++counter;
        }    
        iter++;
    }
   
    std::cout << "Found " << counter << " even numbers in V" << std::endl;

    // random access operator [] 

    const vector_type &cv = V;
    std::cout << cv[1234] << " ";

    return 0;
}
//! [example]
