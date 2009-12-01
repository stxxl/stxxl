/***************************************************************************
 *  include/stxxl/bits/mng/diskallocator.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
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


__STXXL_BEGIN_NAMESPACE

//! \ingroup mnglayer
//! \{

class DiskAllocator : private noncopyable
{
    stxxl::mutex mutex;

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

    struct OffCmp
    {
        bool operator () (const stxxl::int64 & off1, const stxxl::int64 & off2)
        {
            return off1 < off2;
        }
    };

    DiskAllocator()
    { }

protected:
    typedef std::map<stxxl::int64, stxxl::int64> sortseq;
    sortseq free_space;
    //  sortseq used_space;
    stxxl::int64 free_bytes;
    stxxl::int64 disk_bytes;
    bool autogrow;

    void dump();

    void check_corruption(stxxl::int64 region_pos, stxxl::int64 region_size,
                          sortseq::iterator pred, sortseq::iterator succ)
    {
        if (pred != free_space.end())
        {
            if (pred->first <= region_pos && pred->first + pred->second > region_pos)
            {
                STXXL_THROW(bad_ext_alloc, "DiskAllocator::check_corruption", "Error: double deallocation of external memory, trying to deallocate region " << region_pos << " + " << region_size << "  in empty space [" << pred->first << " + " << pred->second << "]");
            }
        }
        if (succ != free_space.end())
        {
            if (region_pos <= succ->first && region_pos + region_size > succ->first)
            {
                STXXL_THROW(bad_ext_alloc, "DiskAllocator::check_corruption", "Error: double deallocation of external memory, trying to deallocate region " << region_pos << " + " << region_size << "  which overlaps empty space [" << succ->first << " + " << succ->second << "]");
            }
        }
    }

public:
    inline DiskAllocator(stxxl::int64 disk_size);

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
    stxxl::int64 new_blocks(BIDArray<BLK_SIZE> & bids);

    template <unsigned BLK_SIZE>
    stxxl::int64 new_blocks(BID<BLK_SIZE> * begin,
                            BID<BLK_SIZE> * end);
#if 0
    template <unsigned BLK_SIZE>
    void delete_blocks(const BIDArray<BLK_SIZE> & bids);
#endif
    template <unsigned BLK_SIZE>
    void delete_block(const BID<BLK_SIZE> & bid);
};

DiskAllocator::DiskAllocator(stxxl::int64 disk_size) :
    free_bytes(disk_size),
    disk_bytes(disk_size),
    autogrow(disk_size == 0)
{
    free_space[0] = disk_size;
}


template <unsigned BLK_SIZE>
stxxl::int64 DiskAllocator::new_blocks(BIDArray<BLK_SIZE> & bids)
{
    return new_blocks(bids.begin(), bids.end());
}

template <unsigned BLK_SIZE>
stxxl::int64 DiskAllocator::new_blocks(BID<BLK_SIZE> * begin,
                                       BID<BLK_SIZE> * end)
{
    scoped_mutex_lock lock(mutex);

    STXXL_VERBOSE2("DiskAllocator::new_blocks<BLK_SIZE>,  BLK_SIZE = " << BLK_SIZE <<
                   ", free:" << free_bytes << " total:" << disk_bytes <<
                   ", blocks: " << (end - begin) <<
                   " begin: " << ((void *)(begin)) << " end: " << ((void *)(end)));

    stxxl::int64 requested_size = 0;

    typename BIDArray<BLK_SIZE>::iterator cur = begin;
    for ( ; cur != end; ++cur)
    {
        STXXL_VERBOSE2("Asking for a block with size: " << (cur->size));
        requested_size += cur->size;
    }

    if (free_bytes < requested_size)
    {
        if (!autogrow) {
            STXXL_ERRMSG("External memory block allocation error: " << requested_size <<
                         " bytes requested, " << free_bytes <<
                         " bytes free. Trying to extend the external memory space...");
        }

        begin->offset = disk_bytes; // allocate at the end
        for (++begin; begin != end; ++begin)
        {
            begin->offset = (begin - 1)->offset + (begin - 1)->size;
        }
        disk_bytes += requested_size;

        return disk_bytes;
    }

    // dump();

    sortseq::iterator space =
        std::find_if(free_space.begin(), free_space.end(),
                     bind2nd(FirstFit(), requested_size) __STXXL_FORCE_SEQUENTIAL);

    if (space != free_space.end())
    {
        stxxl::int64 region_pos = (*space).first;
        stxxl::int64 region_size = (*space).second;
        free_space.erase(space);
        if (region_size > requested_size)
            free_space[region_pos + requested_size] = region_size - requested_size;

        begin->offset = region_pos;
        for (++begin; begin != end; ++begin)
        {
            begin->offset = (begin - 1)->offset + (begin - 1)->size;
        }
        free_bytes -= requested_size;
        //dump();

        return disk_bytes;
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
        disk_bytes += BLK_SIZE;

        return disk_bytes;
    }

    assert(requested_size > BLK_SIZE);
    assert(end - begin > 1);

    lock.unlock();

    typename  BIDArray<BLK_SIZE>::iterator middle = begin + ((end - begin) / 2);
    new_blocks(begin, middle);
    new_blocks(middle, end);

    return disk_bytes;
}


template <unsigned BLK_SIZE>
void DiskAllocator::delete_block(const BID<BLK_SIZE> & bid)
{
    scoped_mutex_lock lock(mutex);

    STXXL_VERBOSE2("DiskAllocator::delete_block<BLK_SIZE>,  BLK_SIZE = " << BLK_SIZE
                                                                         << ", free:" << free_bytes << " total:" << disk_bytes);
    STXXL_VERBOSE2("Deallocating a block with size: " << bid.size);
    //assert(bid.size);

    //dump();
    stxxl::int64 region_pos = bid.offset;
    stxxl::int64 region_size = bid.size;
    STXXL_VERBOSE2("Deallocating a block with size: " << region_size << " position: " << region_pos);
    if (!free_space.empty())
    {
        sortseq::iterator succ = free_space.upper_bound(region_pos);
        sortseq::iterator pred = succ;
        pred--;
        check_corruption(region_pos, region_size, pred, succ);
        if (succ == free_space.end())
        {
            if (pred == free_space.end())
            {
                STXXL_ERRMSG("Error deallocating block at " << bid.offset << " size " << bid.size);
                STXXL_ERRMSG(((pred == succ) ? "pred==succ" : "pred!=succ"));
                STXXL_ERRMSG(((pred == free_space.begin()) ? "pred==free_space.begin()" : "pred!=free_space.begin()"));
                STXXL_ERRMSG(((pred == free_space.end()) ? "pred==free_space.end()" : "pred!=free_space.end()"));
                STXXL_ERRMSG(((succ == free_space.begin()) ? "succ==free_space.begin()" : "succ!=free_space.begin()"));
                STXXL_ERRMSG(((succ == free_space.end()) ? "succ==free_space.end()" : "succ!=free_space.end()"));
                dump();
                assert(pred != free_space.end());
            }
            if ((*pred).first + (*pred).second == region_pos)
            {
                // coalesce with predecessor
                region_size += (*pred).second;
                region_pos = (*pred).first;
                free_space.erase(pred);
            }
        }
        else
        {
            if (free_space.size() > 1)
            {
#if 0
                if (pred == succ)
                {
                    STXXL_ERRMSG("Error deallocating block at " << bid.offset << " size " << bid.size);
                    STXXL_ERRMSG(((pred == succ) ? "pred==succ" : "pred!=succ"));
                    STXXL_ERRMSG(((pred == free_space.begin()) ? "pred==free_space.begin()" : "pred!=free_space.begin()"));
                    STXXL_ERRMSG(((pred == free_space.end()) ? "pred==free_space.end()" : "pred!=free_space.end()"));
                    STXXL_ERRMSG(((succ == free_space.begin()) ? "succ==free_space.begin()" : "succ!=free_space.begin()"));
                    STXXL_ERRMSG(((succ == free_space.end()) ? "succ==free_space.end()" : "succ!=free_space.end()"));
                    dump();
                    assert(pred != succ);
                }
#endif
                bool succ_is_not_the_first = (succ != free_space.begin());
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space.erase(succ);
                }
                if (succ_is_not_the_first)
                {
                    if (pred == free_space.end())
                    {
                        STXXL_ERRMSG("Error deallocating block at " << bid.offset << " size " << bid.size);
                        STXXL_ERRMSG(((pred == succ) ? "pred==succ" : "pred!=succ"));
                        STXXL_ERRMSG(((pred == free_space.begin()) ? "pred==free_space.begin()" : "pred!=free_space.begin()"));
                        STXXL_ERRMSG(((pred == free_space.end()) ? "pred==free_space.end()" : "pred!=free_space.end()"));
                        STXXL_ERRMSG(((succ == free_space.begin()) ? "succ==free_space.begin()" : "succ!=free_space.begin()"));
                        STXXL_ERRMSG(((succ == free_space.end()) ? "succ==free_space.end()" : "succ!=free_space.end()"));
                        dump();
                        assert(pred != free_space.end());
                    }
                    if ((*pred).first + (*pred).second == region_pos)
                    {
                        // coalesce with predecessor
                        region_size += (*pred).second;
                        region_pos = (*pred).first;
                        free_space.erase(pred);
                    }
                }
            }
            else
            {
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space.erase(succ);
                }
            }
        }
    }

    free_space[region_pos] = region_size;
    free_bytes += stxxl::int64(bid.size);

    //dump();
}

#if 0
template <unsigned BLK_SIZE>
void DiskAllocator::delete_blocks(const BIDArray<BLK_SIZE> & bids)
{
    STXXL_VERBOSE2("DiskAllocator::delete_blocks<BLK_SIZE> BLK_SIZE=" << BLK_SIZE <<
                   ", free:" << free_bytes << " total:" << disk_bytes);

    unsigned i = 0;
    for ( ; i < bids.size(); ++i)
    {
        stxxl::int64 region_pos = bids[i].offset;
        stxxl::int64 region_size = bids[i].size;
        STXXL_VERBOSE2("Deallocating a block with size: " << region_size);
        assert(bids[i].size);

        if (!free_space.empty())
        {
            sortseq::iterator succ =
                free_space.upper_bound(region_pos);
            sortseq::iterator pred = succ;
            pred--;

            if (succ != free_space.end()
                && (*succ).first == region_pos + region_size)
            {
                // coalesce with successor

                region_size += (*succ).second;
                free_space.erase(succ);
            }
            if (pred != free_space.end()
                && (*pred).first + (*pred).second == region_pos)
            {
                // coalesce with predecessor

                region_size += (*pred).second;
                region_pos = (*pred).first;
                free_space.erase(pred);
            }
        }
        free_space[region_pos] = region_size;
    }
    for (i = 0; i < bids.size(); ++i)
        free_bytes += stxxl::int64(bids[i].size);
}
#endif

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_DISKALLOCATOR_HEADER
// vim: et:ts=4:sw=4
