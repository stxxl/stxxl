/***************************************************************************
 *  lib/mng/disk_block_allocator.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/mng/disk_block_allocator.h>
#include <stxxl/bits/verbose.h>

#include <cassert>
#include <map>
#include <ostream>
#include <utility>

namespace stxxl {

void disk_block_allocator::dump() const
{
    uint64_t total = 0;
    space_map_type::const_iterator cur = free_space_.begin();
    STXXL_ERRMSG("Free regions dump:");
    for ( ; cur != free_space_.end(); ++cur)
    {
        STXXL_ERRMSG("Free chunk: begin: " << (cur->first) << " size: " << (cur->second));
        total += cur->second;
    }
    STXXL_ERRMSG("Total bytes: " << total);
}

void disk_block_allocator::deallocation_error(
    uint64_t block_pos, uint64_t block_size,
    const space_map_type::iterator& pred, const space_map_type::iterator& succ) const
{
    STXXL_ERRMSG("Error deallocating block at " << block_pos << " size " << block_size);
    STXXL_ERRMSG(((pred == succ) ? "pred==succ" : "pred!=succ"));
    if (pred == free_space_.end()) {
        STXXL_ERRMSG("pred==free_space_.end()");
    }
    else {
        if (pred == free_space_.begin())
            STXXL_ERRMSG("pred==free_space_.begin()");
        STXXL_ERRMSG("pred: begin=" << pred->first << " size=" << pred->second);
    }
    if (succ == free_space_.end()) {
        STXXL_ERRMSG("succ==free_space_.end()");
    }
    else {
        if (succ == free_space_.begin())
            STXXL_ERRMSG("succ==free_space_.begin()");
        STXXL_ERRMSG("succ: begin=" << succ->first << " size=" << succ->second);
    }
    dump();
}

void disk_block_allocator::add_free_region(uint64_t block_pos, uint64_t block_size)
{
    //assert(block_size);
    //dump();
    STXXL_VERBOSE2("Deallocating a block with size: " << block_size << " position: " << block_pos);
    uint64_t region_pos = block_pos;
    uint64_t region_size = block_size;
    if (!free_space_.empty())
    {
        space_map_type::iterator succ = free_space_.upper_bound(region_pos);
        space_map_type::iterator pred = succ;
        if (pred != free_space_.begin())
            pred--;
        if (pred != free_space_.end())
        {
            if (pred->first <= region_pos && pred->first + pred->second > region_pos)
            {
                STXXL_THROW2(bad_ext_alloc, "disk_block_allocator::check_corruption", "Error: double deallocation of external memory, trying to deallocate region " << region_pos << " + " << region_size << "  in empty space [" << pred->first << " + " << pred->second << "]");
            }
        }
        if (succ != free_space_.end())
        {
            if (region_pos <= succ->first && region_pos + region_size > succ->first)
            {
                STXXL_THROW2(bad_ext_alloc, "disk_block_allocator::check_corruption", "Error: double deallocation of external memory, trying to deallocate region " << region_pos << " + " << region_size << "  which overlaps empty space [" << succ->first << " + " << succ->second << "]");
            }
        }
        if (succ == free_space_.end())
        {
            if (pred == free_space_.end())
            {
                deallocation_error(block_pos, block_size, pred, succ);
                assert(pred != free_space_.end());
            }
            if ((*pred).first + (*pred).second == region_pos)
            {
                // coalesce with predecessor
                region_size += (*pred).second;
                region_pos = (*pred).first;
                free_space_.erase(pred);
            }
        }
        else
        {
            if (free_space_.size() > 1)
            {
#if 0
                if (pred == succ)
                {
                    deallocation_error(block_pos, block_size, pred, succ);
                    assert(pred != succ);
                }
#endif
                bool succ_is_not_the_first = (succ != free_space_.begin());
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space_.erase(succ);
                    //-tb: set succ to pred afterwards due to iterator invalidation
                    succ = pred;
                }
                if (succ_is_not_the_first)
                {
                    if (pred == free_space_.end())
                    {
                        deallocation_error(block_pos, block_size, pred, succ);
                        assert(pred != free_space_.end());
                    }
                    if ((*pred).first + (*pred).second == region_pos)
                    {
                        // coalesce with predecessor
                        region_size += (*pred).second;
                        region_pos = (*pred).first;
                        free_space_.erase(pred);
                    }
                }
            }
            else
            {
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space_.erase(succ);
                }
            }
        }
    }

    free_space_[region_pos] = region_size;
    free_bytes_ += block_size;

    //dump();
}

} // namespace stxxl
// vim: et:ts=4:sw=4
