/***************************************************************************
 *  examples/containers/pq1.cpp
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
#include <stxxl/priority_queue>
#include <iostream>

// comparison struct for priority queue where top() returns the smallest contained value:
struct Comparator
{
    bool operator () (const int &a, const int &b) const
    { return (a>b); }

    int min_value() const
    { return (std::numeric_limits<int>::max) (); }

};

int main()
{
    // template parameter <data_type, comparator class, internal_memory_limit, number_of_elements, tuning_parameter>
    typedef stxxl::PRIORITY_QUEUE_GENERATOR<int, Comparator, 64*1024*1024, 1024*1024, 6>::result pq_type;
    typedef pq_type::block_type block_type;

    // predefine resizable buffered writing and prefetched reading pool the priority queue makes use of
    const unsigned int mem_for_pools = 64 * 1024 * 1024;
    stxxl::read_write_pool<block_type> pool((mem_for_pools / 2) / block_type::raw_size, (mem_for_pools / 2) / block_type::raw_size);
    pq_type p(pool);

    STXXL_MSG("internal memory consumption of the priority queue in bytes: " << p.mem_cons() << " bytes");

    int tmp;
    unsigned int elements = 128 * 1024 * 1024;
    stxxl::random_number<> n_rand;

    // fill priority queue with values
    for (unsigned int i = 1; i <= elements; i++)
    {
        tmp = n_rand(2147483647);  // generate random number in [0,2147483647)
        p.push(tmp);
        //std::cout << tmp << " ";
    }

    STXXL_MSG("number of elements contained: " << p.size());

    // erase 1000 values from top and return smallest value which remains in the priority queue
    for (int k = 0; k < 1000; k++) { p.pop(); }
    STXXL_MSG("element on top: " << p.top());

    // pop values till emptyness
    while (!p.empty()) {
        tmp = p.top();
        p.pop();
        //std::cout << tmp << " ";
    }

    return 0;
}
//! [example]
