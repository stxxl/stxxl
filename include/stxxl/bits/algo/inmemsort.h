/***************************************************************************
 *  include/stxxl/bits/algo/inmemsort.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_INMEMSORT_HEADER
#define STXXL_ALGO_INMEMSORT_HEADER

#include <foxxll/io/request_operations.hpp>
#include <stxxl/bits/algo/bid_adapter.h>
#include <stxxl/bits/algo/trigger_entry.h>
#include <stxxl/bits/parallel.h>
#include <tlx/simple_vector.hpp>

#include <algorithm>

namespace stxxl {

template <typename ExtIterator, typename StrictWeakOrdering>
void stl_in_memory_sort(ExtIterator first, ExtIterator last, StrictWeakOrdering cmp)
{
    using block_type = typename ExtIterator::block_type;

    LOG1 << "stl_in_memory_sort, range: " << (last - first);
    first.flush();
    size_t nblocks = last.bid() - first.bid() + (last.block_offset() ? 1 : 0);
    tlx::simple_vector<block_type> blocks(nblocks);
    tlx::simple_vector<foxxll::request_ptr> reqs(nblocks);
    size_t i;

    for (i = 0; i < nblocks; ++i)
        reqs[i] = blocks[i].read(*(first.bid() + i));

    wait_all(reqs.begin(), nblocks);

    size_t last_block_correction = last.block_offset() ? (block_type::size - last.block_offset()) : 0;
    check_sort_settings();
    potentially_parallel::
    sort(make_element_iterator(blocks.begin(), first.block_offset()),
         make_element_iterator(blocks.begin(), nblocks * block_type::size - last_block_correction),
         cmp);

    for (i = 0; i < nblocks; ++i)
        reqs[i] = blocks[i].write(*(first.bid() + i));

    wait_all(reqs.begin(), nblocks);
}

} // namespace stxxl

#endif // !STXXL_ALGO_INMEMSORT_HEADER
