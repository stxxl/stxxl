/***************************************************************************
 *  include/stxxl/bits/mng/disk_allocator.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013-2015 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_DISK_ALLOCATOR_HEADER
#define STXXL_MNG_DISK_ALLOCATOR_HEADER

#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/mng/bid.h>
#include <stxxl/bits/mng/config.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/bits/verbose.h>

#include <mutex>
#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <ostream>
#include <utility>

namespace stxxl {

//! \ingroup mnglayer
//! \{

class disk_allocator
{
    typedef std::pair<stxxl::int64, stxxl::int64> place;

    struct first_fit : public std::binary_function<place, stxxl::int64, bool>
    {
        bool operator () (
            const place& entry,
            const stxxl::int64 size) const
        {
            return (entry.second >= size);
        }
    };

    typedef std::map<stxxl::int64, stxxl::int64> sortseq;

    std::mutex mutex;
    sortseq free_space;
    stxxl::int64 free_bytes;
    stxxl::int64 disk_bytes;
    stxxl::int64 cfg_bytes;
    stxxl::file* storage;
    bool autogrow;

    void dump() const;

    void deallocation_error(
        stxxl::int64 block_pos, stxxl::int64 block_size,
        const sortseq::iterator& pred, const sortseq::iterator& succ) const;

    // expects the mutex to be locked to prevent concurrent access
    void add_free_region(stxxl::int64 block_pos, stxxl::int64 block_size);

    // expects the mutex to be locked to prevent concurrent access
    void grow_file(stxxl::int64 extend_bytes)
    {
        if (!extend_bytes)
            return;

        storage->set_size(disk_bytes + extend_bytes);
        add_free_region(disk_bytes, extend_bytes);
        disk_bytes += extend_bytes;
    }

public:
    disk_allocator(stxxl::file* storage, const disk_config& cfg)
        : free_bytes(0),
          disk_bytes(0),
          cfg_bytes(cfg.size),
          storage(storage),
          autogrow(cfg.autogrow)
    {
        // initial growth to configured file size
        grow_file(cfg.size);
    }

    //! non-copyable: delete copy-constructor
    disk_allocator(const disk_allocator&) = delete;
    //! non-copyable: delete assignment operator
    disk_allocator& operator = (const disk_allocator&) = delete;

    ~disk_allocator()
    {
        if (disk_bytes > cfg_bytes) { // reduce to original size
            storage->set_size(cfg_bytes);
        }
    }

    inline int64 get_free_bytes() const
    {
        return free_bytes;
    }

    inline int64 get_used_bytes() const
    {
        return disk_bytes - free_bytes;
    }

    inline int64 get_total_bytes() const
    {
        return disk_bytes;
    }

    template <size_t BlockSize>
    void new_blocks(BIDArray<BlockSize>& bids)
    {
        new_blocks(bids.begin(), bids.end());
    }

    template <size_t BlockSize>
    void new_blocks(BID<BlockSize>* begin, BID<BlockSize>* end);

#if 0
    template <size_t BlockSize>
    void delete_blocks(const BIDArray<BlockSize>& bids)
    {
        for (size_t i = 0; i < bids.size(); ++i)
            delete_block(bids[i]);
    }
#endif

    template <size_t BlockSize>
    void delete_block(const BID<BlockSize>& bid)
    {
        std::unique_lock<std::mutex> lock(mutex);

        STXXL_VERBOSE2("disk_allocator::delete_block<" << BlockSize <<
                       ">(pos=" << bid.offset << ", size=" << bid.size <<
                       "), free:" << free_bytes << " total:" << disk_bytes);

        add_free_region(bid.offset, bid.size);
    }
};

template <size_t BlockSize>
void disk_allocator::new_blocks(BID<BlockSize>* begin, BID<BlockSize>* end)
{
    size_t requested_size = 0;

    for (typename BIDArray<BlockSize>::iterator cur = begin; cur != end; ++cur)
    {
        STXXL_VERBOSE2("Asking for a block with size: " << (cur->size));
        requested_size += cur->size;
    }

    std::unique_lock<std::mutex> lock(mutex);

    STXXL_VERBOSE2("disk_allocator::new_blocks<BlockSize>,  BlockSize = " << BlockSize <<
                   ", free:" << free_bytes << " total:" << disk_bytes <<
                   ", blocks: " << (end - begin) <<
                   " begin: " << static_cast<void*>(begin) <<
                   " end: " << static_cast<void*>(end) <<
                   ", requested_size=" << requested_size);

    if (free_bytes < static_cast<int64_t>(requested_size))
    {
        if (!autogrow) {
            STXXL_THROW(bad_ext_alloc,
                        "Out of external memory error: " << requested_size <<
                        " requested, " << free_bytes << " bytes free. "
                        "Maybe enable autogrow flags?");
        }

        STXXL_ERRMSG("External memory block allocation error: " << requested_size <<
                     " bytes requested, " << free_bytes <<
                     " bytes free. Trying to extend the external memory space...");

        grow_file(requested_size);
    }

    // dump();

    sortseq::iterator space;
    space = std::find_if(free_space.begin(), free_space.end(),
                         bind2nd(first_fit(), requested_size) _STXXL_FORCE_SEQUENTIAL);

    if (space == free_space.end() && requested_size == BlockSize)
    {
        assert(end - begin == 1);

        if (!autogrow) {
            STXXL_ERRMSG("Warning: Severe external memory space fragmentation!");
            dump();

            STXXL_ERRMSG("External memory block allocation error: " << requested_size <<
                         " bytes requested, " << free_bytes <<
                         " bytes free. Trying to extend the external memory space...");
        }

        grow_file(BlockSize);

        space = std::find_if(free_space.begin(), free_space.end(),
                             bind2nd(first_fit(), requested_size) _STXXL_FORCE_SEQUENTIAL);
    }

    if (space != free_space.end())
    {
        stxxl::int64 region_pos = (*space).first;
        stxxl::int64 region_size = (*space).second;
        free_space.erase(space);
        if (region_size > static_cast<int64_t>(requested_size))
            free_space[region_pos + requested_size] = region_size - requested_size;

        for (stxxl::int64 pos = region_pos; begin != end; ++begin)
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

    assert(requested_size > BlockSize);
    assert(end - begin > 1);

    lock.unlock();

    BID<BlockSize>* middle = begin + ((end - begin) / 2);
    new_blocks(begin, middle);
    new_blocks(middle, end);
}

//! \}

} // namespace stxxl

#endif // !STXXL_MNG_DISK_ALLOCATOR_HEADER
// vim: et:ts=4:sw=4
