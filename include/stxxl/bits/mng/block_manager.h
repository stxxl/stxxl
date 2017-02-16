/***************************************************************************
 *  include/stxxl/bits/mng/block_manager.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_BLOCK_MANAGER_HEADER
#define STXXL_MNG_BLOCK_MANAGER_HEADER

#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/defines.h>
#include <stxxl/bits/deprecated.h>
#include <stxxl/bits/io/create_file.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/mng/bid.h>
#include <stxxl/bits/mng/block_alloc_strategy.h>
#include <stxxl/bits/mng/config.h>
#include <stxxl/bits/mng/disk_block_allocator.h>
#include <stxxl/bits/singleton.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if STXXL_MSVC
#include <memory.h>
#endif

namespace stxxl {

//! \addtogroup mnglayer
//! \{

/*!
 * Block manager class.
 *
 * Manages allocation and deallocation of blocks in multiple/single disk setting
 * \remarks is a singleton
 */
class block_manager : public singleton<block_manager>
{
public:
    /*!
     * Allocates new blocks.
     *
     * Allocates new blocks according to the strategy given by \b functor and
     * stores block identifiers to the range [ \b bid_begin, \b bid_end)
     * Allocation will be lined up with previous partial allocations of \b
     * alloc_offset blocks. For BID<0> allocations, the objects' size field must
     * be initialized.
     *
     * \param functor object of model of \b allocation_strategy concept
     * \param bid_begin bidirectional BID iterator object
     * \param bid_end bidirectional BID iterator object
     * \param alloc_offset advance for \b functor to line up partial allocations
     */
    template <typename DiskAssignFunctor, typename BIDIterator>
    void new_blocks(
        const DiskAssignFunctor& functor,
        BIDIterator bid_begin, BIDIterator bid_end,
        size_t alloc_offset = 0);

    /*!
     * Allocates a new block according to the strategy given by \b functor and
     * stores the block identifier to bid.
     *
     * Allocation will be lined up with previous partial allocations of \b
     * alloc_offset blocks.
     *
     * \param functor object of model of \b allocation_strategy concept
     * \param bid BID to store the block identifier
     * \param alloc_offset advance for \b functor to line up partial allocations
     */
    template <typename DiskAssignFunctor, size_t BlockSize>
    void new_block(const DiskAssignFunctor& functor,
                   BID<BlockSize>& bid, size_t alloc_offset = 0)
    {
        new_blocks(functor, &(&bid)[0], &(&bid)[1], alloc_offset);
    }

    //! Deallocates blocks.
    //!
    //! Deallocates blocks in the range [ \b bid_begin, \b bid_end)
    //! \param bid_begin iterator object of \b bid_iterator concept
    //! \param bid_end iterator object of \b bid_iterator concept
    template <typename BIDIterator>
    void delete_blocks(const BIDIterator& bid_begin,
                       const BIDIterator& bid_end);

    //! Deallocates a block.
    //! \param bid block identifier
    template <size_t BlockSize>
    void delete_block(const BID<BlockSize>& bid);

    //! \name Statistics
    //! \{

    //! return total number of bytes available in all disks
    uint64_t total_bytes() const;

    //! Return total number of free bytes
    uint64_t free_bytes() const;

    //! return total requested allocation in bytes
    uint64_t total_allocation() const;

    //! return currently allocated bytes
    uint64_t current_allocation() const;

    //! return maximum number of bytes allocated during program run.
    uint64_t maximum_allocation() const;

    //! \}

    ~block_manager();

private:
    friend class singleton<block_manager>;

    //! number of managed disks
    size_t ndisks_;

    //! vector of opened disk files
    simple_vector<file_ptr> disk_files_;

    //! one block allocator per disk
    simple_vector<disk_block_allocator*> block_allocators_;

    //! total requested allocation in bytes
    uint64_t total_allocation_ = 0;

    //! currently allocated bytes
    uint64_t current_allocation_ = 0;

    //! maximum number of bytes allocated during program run.
    uint64_t maximum_allocation_ = 0;

    //! private construction from singleton
    block_manager();

    //! protect internal data structures
    mutable std::mutex mutex_;
};

template <typename DiskAssignFunctor, typename BIDIterator>
void block_manager::new_blocks(
    const DiskAssignFunctor& functor,
    BIDIterator bid_begin, BIDIterator bid_end,
    size_t alloc_offset)
{
    std::unique_lock<std::mutex> lock(mutex_);

    using BIDType = typename std::iterator_traits<BIDIterator>::value_type;

    // choose disks for each block, sum up bytes allocated on a disk

    simple_vector<size_t> disk_blocks(ndisks_);
    simple_vector<uint64_t> disk_bytes(ndisks_);
    std::vector<std::vector<size_t> > disk_out(ndisks_);

    disk_blocks.memzero();
    disk_bytes.memzero();

    size_t bid_size = static_cast<size_t>(bid_end - bid_begin);

    BIDIterator bid = bid_begin;
    for (size_t i = 0; i < bid_size; ++i, ++bid)
    {
        size_t disk_id = functor(alloc_offset + i);

        if (!block_allocators_[disk_id]->has_available_space(
                disk_bytes[disk_id] + bid->size))
        {
            // find disk (cyclically) that has enough free space for block

            for (size_t adv = 1; adv < ndisks_; ++adv)
            {
                size_t try_disk_id = (disk_id + adv) % ndisks_;
                if (block_allocators_[try_disk_id]->has_available_space(
                        disk_bytes[try_disk_id] + bid->size))
                {
                    disk_id = try_disk_id;
                    break;
                }
            }

            // if no disk has free space, pick first selected by functor
        }

        // assign block to disk
        disk_blocks[disk_id]++;
        disk_bytes[disk_id] += bid->size;
        disk_out[disk_id].push_back(i);
    }

    // allocate blocks on disks in sequence, then scatter blocks into output

    simple_vector<BIDType> bids;

    for (size_t d = 0; d < ndisks_; ++d)
    {
        if (disk_blocks[d] == 0) continue;
        bids.resize(disk_blocks[d]);

        std::vector<size_t>& bid_perm = disk_out[d];

        // collect bids from output (due to size field for BID<0>)
        for (size_t i = 0; i < disk_blocks[d]; ++i)
            bids[i] = bid_begin[bid_perm[i]];

        // let block_allocator fill in offset fields
        block_allocators_[d]->new_blocks(bids);

        // distributed bids back to output
        for (size_t i = 0; i < disk_blocks[d]; ++i) {
            bids[i].storage = disk_files_[d].get();

            STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:new    " << FMT_BID(bids[i]));
            bid_begin[bid_perm[i]] = bids[i];

            total_allocation_ += bids[i].size;
            current_allocation_ += bids[i].size;
        }
    }

    maximum_allocation_ = std::max(maximum_allocation_, current_allocation_);
}

template <size_t BlockSize>
void block_manager::delete_block(const BID<BlockSize>& bid)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (!bid.valid()) {
        //STXXL_MSG("Warning: invalid block to be deleted.");
        return;
    }
    if (!bid.is_managed())
        return;  // self managed disk
    STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:delete " << FMT_BID(bid));
    assert(bid.storage->get_allocator_id() >= 0);
    block_allocators_[bid.storage->get_allocator_id()]->delete_block(bid);
    disk_files_[bid.storage->get_allocator_id()]->discard(bid.offset, bid.size);

    current_allocation_ -= BlockSize;
}

template <typename BIDIterator>
void block_manager::delete_blocks(
    const BIDIterator& bid_begin, const BIDIterator& bid_end)
{
    for (BIDIterator it = bid_begin; it != bid_end; ++it)
        delete_block(*it);
}

// in bytes
#ifndef STXXL_DEFAULT_BLOCK_SIZE
// ensure that the type parameter to STXXL_DEFAULT_BLOCK_SIZE is a valid type
template <typename T>
struct dummy_default_block_size
{
    static constexpr size_t size()
    {
        return 2 * 1024 * 1024;
    }
};
    #define STXXL_DEFAULT_BLOCK_SIZE(type) (size_t(stxxl::dummy_default_block_size<type>::size())) // use traits
#endif

//! \}

} // namespace stxxl

#endif // !STXXL_MNG_BLOCK_MANAGER_HEADER
// vim: et:ts=4:sw=4
