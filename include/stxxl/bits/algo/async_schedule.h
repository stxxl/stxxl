/***************************************************************************
 *  include/stxxl/bits/algo/async_schedule.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_ASYNC_SCHEDULE_HEADER
#define STXXL_ALGO_ASYNC_SCHEDULE_HEADER

// Implements the "prudent prefetching" as described in
// D. Hutchinson, P. Sanders, J. S. Vitter: Duality between prefetching
// and queued writing on parallel disks, 2005
// DOI: 10.1137/S0097539703431573

#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/common/types.h>

namespace stxxl {

void compute_prefetch_schedule(
    const size_t* first,
    const size_t* last,
    size_t* out_first,
    size_t m,
    size_t D);

inline void compute_prefetch_schedule(
    size_t* first,
    size_t* last,
    size_t* out_first,
    size_t m,
    size_t D)
{
    compute_prefetch_schedule(static_cast<const size_t*>(first), last, out_first, m, D);
}

template <typename RunType>
void compute_prefetch_schedule(
    const RunType& input,
    size_t* out_first,
    size_t m,
    size_t D)
{
    const size_t L = input.size();
    simple_vector<size_t> disks(L);
    for (size_t i = 0; i < L; ++i)
        disks[i] = input[i].bid.storage->get_device_id();
    compute_prefetch_schedule(disks.begin(), disks.end(), out_first, m, D);
}

template <typename BidIteratorType>
void compute_prefetch_schedule(
    BidIteratorType input_begin,
    BidIteratorType input_end,
    size_t* out_first,
    size_t m,
    size_t D)
{
    const size_t L = input_end - input_begin;
    simple_vector<size_t> disks(L);
    size_t i = 0;
    for (BidIteratorType it = input_begin; it != input_end; ++it, ++i)
        disks[i] = it->storage->get_device_id();
    compute_prefetch_schedule(disks.begin(), disks.end(), out_first, m, D);
}

} // namespace stxxl

#endif // !STXXL_ALGO_ASYNC_SCHEDULE_HEADER
// vim: et:ts=4:sw=4
