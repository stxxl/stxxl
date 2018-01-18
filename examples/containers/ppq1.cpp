/***************************************************************************
 *  examples/containers/ppq1.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2015 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! [example]
#include <tlx/logger.hpp>

#include <stxxl/parallel_priority_queue>

#if STXXL_PARALLEL
#include <omp.h>
#endif
#include <iostream>

int main()
{
    using comparator_type = std::greater<unsigned>;
    using ppq_type = stxxl::parallel_priority_queue<unsigned, comparator_type>;
    ppq_type ppq(comparator_type(), 128L * 1024L * 1024L);

    // sequential access

    LOG1 << "Pushing values 5, 4, 19, 1...";

    ppq.push(5);
    ppq.push(4);
    ppq.push(19);
    ppq.push(1);

    assert(ppq.size() == 4);
    assert(ppq.top() == 1);
    LOG1 << "Size: " << ppq.size();
    LOG1 << "Smallest value: " << ppq.top();

    LOG1 << "Extracting 1 value...";
    ppq.pop();  // pop the 1 on top

    assert(ppq.size() == 3);
    assert(ppq.top() == 4);
    LOG1 << "Size: " << ppq.size();
    LOG1 << "Smallest value: " << ppq.top();

    // bulk push

    LOG1 << "Bulk-pushing values 0 to 10000...";

    ppq.bulk_push_begin(10000);
#if STXXL_PARALLEL
    #pragma omp parallel for
#endif
    for (unsigned i = 0; i < 10000; ++i)
    {
#if STXXL_PARALLEL
        const unsigned thread_id = static_cast<unsigned>(omp_get_thread_num());
#else
        const unsigned thread_id = 0;
#endif
        ppq.bulk_push(i, thread_id);
    }
    ppq.bulk_push_end();

    assert(ppq.size() == 10003);
    assert(ppq.top() == 0);
    LOG1 << "Size: " << ppq.size();
    LOG1 << "Smallest value: " << ppq.top();

    // bulk pop

    LOG1 << "Bulk-extracting 500 values...";

    std::vector<unsigned> out1;
    ppq.bulk_pop(out1, 500);

#if STXXL_PARALLEL
    #pragma omp parallel for
#endif
    for (int64_t i = 0; i < (int64_t)out1.size(); ++i)
    {
        // process out[i]
    }

    assert(out1.size() == 500);
    assert(ppq.size() == 9503);
    assert(ppq.top() == 500 - 3); // -3 because of items 5, 4, and 19.
    LOG1 << "Output size: " << out1.size();
    LOG1 << "Size: " << ppq.size();
    LOG1 << "Smallest value: " << ppq.top();

    // bulk limit

    LOG1 << "Bulk-extracting all values smaller or equal to 8000...";

    std::vector<unsigned> out2;
    unsigned limit_item = 8000;
    ppq.bulk_pop_limit(out2, limit_item);

#if STXXL_PARALLEL
    #pragma omp parallel for
#endif
    for (int64_t i = 0; i < (int64_t)out2.size(); ++i)
    {
        // process out[i]
    }

    assert(out2.size() == 7504);
    assert(ppq.size() == 1999);
    assert(ppq.top() == 8001);
    LOG1 << "Output size: " << out2.size();
    LOG1 << "Size: " << ppq.size();
    LOG1 << "Smallest value: " << ppq.top();

    return EXIT_SUCCESS;
}
//! [example]
