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

#include <stxxl/bits/defines.h>
#include <stxxl/bits/deprecated.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/create_file.h>
#include <stxxl/bits/singleton.h>
#include <stxxl/bits/mng/bid.h>
#include <stxxl/bits/mng/disk_block_allocator.h>
#include <stxxl/bits/mng/block_alloc_strategy.h>
#include <stxxl/bits/mng/config.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/config.h>

#include <memory>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <mutex>

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
     * offset blocks.
     *
     * \param functor object of model of \b allocation_strategy concept
     * \param bid_begin bidirectional BID iterator object
     * \param bid_end bidirectional BID iterator object
     * \param offset advance for \b functor to line up partial allocations
     */
    template <typename DiskAssignFunctor, typename BIDIterator>
    void new_blocks(
        const DiskAssignFunctor& functor,
        BIDIterator bid_begin, BIDIterator bid_end,
        size_t offset = 0)
    {
        typedef typename std::iterator_traits<BIDIterator>::value_type bid_type;
        new_blocks_int<bid_type>(
            std::distance(bid_begin, bid_end), functor, offset, bid_begin);
    }

    /*!
     * Allocates new blocks according to the strategy given by \b functor and
     * stores block identifiers to the output iterator \b out Allocation will be
     * lined up with previous partial allocations of \b offset blocks.
     *
     * \param nblocks the number of blocks to allocate
     * \param functor object of model of \b allocation_strategy concept
     * \param out iterator object of OutputIterator concept
     * \param offset advance for \b functor to line up partial allocations
     *
     * The \c BlockType template parameter defines the type of block to allocate
     */
    template <typename BlockType, typename DiskAssignFunctor,
              typename BIDIterator>
    void new_blocks(
        const size_t nblocks, const DiskAssignFunctor& functor,
        BIDIterator out, size_t offset = 0)
    {
        typedef typename BlockType::bid_type bid_type;
        new_blocks_int<bid_type>(nblocks, functor, offset, out);
    }

    /*!
     * Allocates a new block according to the strategy given by \b functor and
     * stores the block identifier to bid.
     *
     * Allocation will be lined up with previous partial allocations of \b
     * offset blocks.
     *
     * \param functor object of model of \b allocation_strategy concept
     * \param bid BID to store the block identifier
     * \param offset advance for \b functor to line up partial allocations
     */
    template <typename DiskAssignFunctor, size_t BlockSize>
    void new_block(const DiskAssignFunctor& functor,
                   BID<BlockSize>& bid, size_t offset = 0)
    {
        new_blocks_int<BID<BlockSize> >(1, functor, offset, &bid);
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

    //! actual allocation function
    template <typename BIDType, typename DiskAssignFunctor, typename BIDIterator>
    void new_blocks_int(
        const size_t nblocks, const DiskAssignFunctor& functor,
        size_t offset, BIDIterator out);
};

template <typename BIDType, typename DiskAssignFunctor, typename OutputIterator>
void block_manager::new_blocks_int(
    const size_t nblocks, const DiskAssignFunctor& functor,
    size_t offset, OutputIterator out)
{
    std::unique_lock<std::mutex> lock(mutex_);

    typedef BIDType bid_type;
    typedef BIDArray<bid_type::t_size> bid_array_type;

    simple_vector<int_type> bl(ndisks_);
    simple_vector<bid_array_type> disk_bids(ndisks_);
    simple_vector<file*> disk_ptrs(nblocks);

    // choose disks by calling DiskAssignFunctor

    bl.memzero();
    for (size_t i = 0; i < nblocks; ++i)
    {
        size_t disk = functor(offset + i);
        disk_ptrs[i] = disk_files_[disk].get();
        bl[disk]++;
    }

    // allocate blocks on disks

    for (size_t i = 0; i < ndisks_; ++i)
    {
        if (bl[i])
        {
            disk_bids[i].resize(bl[i]);
            block_allocators_[i]->new_blocks(disk_bids[i]);
        }
    }

    bl.memzero();

    OutputIterator it = out;
    for (size_t i = 0; i != nblocks; ++it, ++i)
    {
        const int disk = disk_ptrs[i]->get_allocator_id();
        bid_type bid(disk_ptrs[i], disk_bids[disk][bl[disk]++].offset);
        *it = bid;
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:new    " << FMT_BID(bid));
    }

    total_allocation_ += nblocks * BIDType::size;
    current_allocation_ += nblocks * BIDType::size;
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
