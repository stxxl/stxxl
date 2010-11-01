/***************************************************************************
 *  include/stxxl/bits/mng/diskallocator.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_DISKALLOCATOR_HEADER
#define STXXL_DISKALLOCATOR_HEADER

#include <vector>
#include <map>
#include <algorithm>

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/bits/mng/bid.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/bits/io/file.h>


__STXXL_BEGIN_NAMESPACE

//! \ingroup mnglayer
//! \{

class DiskAllocator : private noncopyable
{
    typedef std::pair<stxxl::int64, stxxl::int64> place;

    struct FirstFit : public std::binary_function<place, stxxl::int64, bool>
    {
        bool operator () (
            const place & entry,
            const stxxl::int64 size) const
        {
            return (entry.second >= size);
        }
    };

    typedef std::map<stxxl::int64, stxxl::int64> sortseq;

    stxxl::mutex mutex;
    sortseq free_space;
    stxxl::int64 free_bytes;
    stxxl::int64 disk_bytes;
    stxxl::file * storage;
    bool autogrow;

    void dump() const;

    void deallocation_error(
        stxxl::int64 block_pos, stxxl::int64 block_size,
        const sortseq::iterator & pred, const sortseq::iterator & succ) const;

    // expects the mutex to be locked
    void add_free_region(stxxl::int64 block_pos, stxxl::int64 block_size);

    // expects the mutex to be locked
    void grow_file(stxxl::int64 extend_bytes)
    {
        if (!extend_bytes)
            return;

        disk_bytes += extend_bytes;
        free_bytes += extend_bytes;
        storage->set_size(disk_bytes);
    }

public:
    DiskAllocator(stxxl::file * storage, stxxl::int64 disk_size) :
        free_bytes(0),
        disk_bytes(0),
        storage(storage),
        autogrow(disk_size == 0)
    {
        grow_file(disk_size);
        free_space[0] = disk_size;
    }

    inline stxxl::int64 get_free_bytes() const
    {
        return free_bytes;
    }

    inline stxxl::int64 get_used_bytes() const
    {
        return disk_bytes - free_bytes;
    }

    inline stxxl::int64 get_total_bytes() const
    {
        return disk_bytes;
    }

    template <unsigned BLK_SIZE>
    void new_blocks(BIDArray<BLK_SIZE> & bids)
    {
        new_blocks(bids.begin(), bids.end());
    }

    template <unsigned BLK_SIZE>
    void new_blocks(BID<BLK_SIZE> * begin, BID<BLK_SIZE> * end);

#if 0
    template <unsigned BLK_SIZE>
    void delete_blocks(const BIDArray<BLK_SIZE> & bids)
    {
        for (unsigned i = 0 ; i < bids.size(); ++i)
            delete_block(bids[i]);
    }
#endif

    template <unsigned BLK_SIZE>
    void delete_block(const BID<BLK_SIZE> & bid)
    {
        scoped_mutex_lock lock(mutex);

        STXXL_VERBOSE2("DiskAllocator::delete_block<" << BLK_SIZE <<
                       ">(pos=" << bid.offset << ", size=" << bid.size <<
                       "), free:" << free_bytes << " total:" << disk_bytes);

        add_free_region(bid.offset, bid.size);
    }
};

template <unsigned BLK_SIZE>
void DiskAllocator::new_blocks(BID<BLK_SIZE> * begin, BID<BLK_SIZE> * end)
{
    stxxl::int64 requested_size = 0;

    for (typename BIDArray<BLK_SIZE>::iterator cur = begin; cur != end; ++cur)
    {
        STXXL_VERBOSE2("Asking for a block with size: " << (cur->size));
        requested_size += cur->size;
    }

    scoped_mutex_lock lock(mutex);

    STXXL_VERBOSE2("DiskAllocator::new_blocks<BLK_SIZE>,  BLK_SIZE = " << BLK_SIZE <<
                   ", free:" << free_bytes << " total:" << disk_bytes <<
                   ", blocks: " << (end - begin) <<
                   " begin: " << static_cast<void *>(begin) <<
                   " end: " << static_cast<void *>(end) <<
                   ", requested_size=" << requested_size);

    if (free_bytes < requested_size)
    {
        if (!autogrow) {
            STXXL_ERRMSG("External memory block allocation error: " << requested_size <<
                         " bytes requested, " << free_bytes <<
                         " bytes free. Trying to extend the external memory space...");
        }

        stxxl::int64 pos = disk_bytes;  // allocate at the end
        grow_file(requested_size);
        for ( ; begin != end; ++begin)
        {
            begin->offset = pos;
            pos += begin->size;
        }
        free_bytes -= requested_size;

        return;
    }

    // dump();

    sortseq::iterator space =
        std::find_if(free_space.begin(), free_space.end(),
                     bind2nd(FirstFit(), requested_size) _STXXL_FORCE_SEQUENTIAL);

    if (space != free_space.end())
    {
        stxxl::int64 region_pos = (*space).first;
        stxxl::int64 region_size = (*space).second;
        free_space.erase(space);
        if (region_size > requested_size)
            free_space[region_pos + requested_size] = region_size - requested_size;

        stxxl::int64 pos = region_pos;
        for ( ; begin != end; ++begin)
        {
            begin->offset = pos;
            pos += begin->size;
        }
        free_bytes -= requested_size;
        //dump();

        return;
    }

    // no contiguous region found
    STXXL_VERBOSE1("Warning, when allocating an external memory space, no contiguous region found");
    STXXL_VERBOSE1("It might harm the performance");
    if (requested_size == BLK_SIZE)
    {
        assert(end - begin == 1);

        STXXL_ERRMSG("Warning: Severe external memory space fragmentation!");
        dump();

        STXXL_ERRMSG("External memory block allocation error: " << requested_size <<
                     " bytes requested, " << free_bytes <<
                     " bytes free. Trying to extend the external memory space...");

        begin->offset = disk_bytes; // allocate at the end
        grow_file(BLK_SIZE);
        free_bytes -= BLK_SIZE;

        return;
    }

    assert(requested_size > BLK_SIZE);
    assert(end - begin > 1);

    lock.unlock();

    typename BIDArray<BLK_SIZE>::iterator middle = begin + ((end - begin) / 2);
    new_blocks(begin, middle);
    new_blocks(middle, end);
}



//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_DISKALLOCATOR_HEADER
// vim: et:ts=4:sw=4
