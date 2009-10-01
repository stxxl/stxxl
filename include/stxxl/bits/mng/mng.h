/***************************************************************************
 *  include/stxxl/bits/mng/mng.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008-2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_HEADER
#define STXXL_MNG_HEADER

#include <memory>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <cstdlib>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC
#include <memory.h>
#endif

#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/singleton.h>
#include <stxxl/bits/mng/bid.h>
#include <stxxl/bits/mng/diskallocator.h>
#include <stxxl/bits/mng/block_alloc.h>
#include <stxxl/bits/mng/config.h>


__STXXL_BEGIN_NAMESPACE

//! \defgroup mnglayer Block management layer
//! Group of classes which help controlling external memory space,
//! managing disks, and allocating and deallocating blocks of external storage
//! \{

//! \brief Block manager class

//! Manages allocation and deallocation of blocks in multiple/single disk setting
//! \remarks is a singleton
class block_manager : public singleton<block_manager>
{
    friend class singleton<block_manager>;

    DiskAllocator ** disk_allocators;
    file ** disk_files;

    unsigned ndisks;
    block_manager();

protected:
    template <class BIDType, class DiskAssignFunctor, class BIDIteratorClass>
    void new_blocks_int(
        const unsigned_type nblocks,
        const DiskAssignFunctor & functor,
        BIDIteratorClass out);

public:
    //! \brief Allocates new blocks

    //! Allocates new blocks according to the strategy
    //! given by \b functor and stores block identifiers
    //! to the range [ \b bidbegin, \b bidend)
    //! \param functor object of model of \b allocation_strategy concept
    //! \param bidbegin bidirectional BID iterator object
    //! \param bidend bidirectional BID iterator object
    template <class DiskAssignFunctor, class BIDIteratorClass>
    void new_blocks(
        const DiskAssignFunctor & functor,
        BIDIteratorClass bidbegin,
        BIDIteratorClass bidend);

    //! Allocates new blocks according to the strategy
    //! given by \b functor and stores block identifiers
    //! to the output iterator \b out
    //! \param nblocks the number of blocks to allocate
    //! \param functor object of model of \b allocation_strategy concept
    //! \param out iterator object of OutputIterator concept
    //!
    //! The \c BlockType template parameter defines the type of block to allocate
    template <class BlockType, class DiskAssignFunctor, class BIDIteratorClass>
    void new_blocks(
        const unsigned_type nblocks,
        const DiskAssignFunctor & functor,
        BIDIteratorClass out);

    //! Allocates a new block according to the strategy
    //! given by \b functor and stores the block identifier
    //! to bid.
    //! \param functor object of model of \b allocation_strategy concept
    //! \param bid BID to store the block identifier
    template <typename DiskAssignFunctor, unsigned BLK_SIZE>
    void new_block(const DiskAssignFunctor & functor, BID<BLK_SIZE> & bid)
    {
        new_blocks_int<BID<BLK_SIZE> >(1, functor, &bid);
    }

    //! \brief Deallocates blocks

    //! Deallocates blocks in the range [ \b bidbegin, \b bidend)
    //! \param bidbegin iterator object of \b bid_iterator concept
    //! \param bidend iterator object of \b bid_iterator concept
    template <class BIDIteratorClass>
    void delete_blocks(const BIDIteratorClass & bidbegin, const BIDIteratorClass & bidend);

    //! \brief Deallocates a block
    //! \param bid block identifier
    template <unsigned BLK_SIZE>
    void delete_block(const BID<BLK_SIZE> & bid);

    ~block_manager();
};


template <class BIDType, class DiskAssignFunctor, class OutputIterator>
void block_manager::new_blocks_int(
    const unsigned_type nblocks,
    const DiskAssignFunctor & functor,
    OutputIterator out)
{
    typedef BIDType bid_type;
    typedef BIDArray<bid_type::t_size> bid_array_type;

    int_type * bl = new int_type[ndisks];
    bid_array_type * disk_bids = new bid_array_type[ndisks];
    file ** disk_ptrs = new file *[nblocks];

    memset(bl, 0, ndisks * sizeof(int_type));

    unsigned_type i;
    for (i = 0; i < nblocks; ++i)
    {
        const int disk = functor(i);
        disk_ptrs[i] = disk_files[disk];
        bl[disk]++;
    }

    for (i = 0; i < ndisks; ++i)
    {
        if (bl[i])
        {
            disk_bids[i].resize(bl[i]);
            const stxxl::int64 old_capacity =
                disk_allocators[i]->get_total_bytes();
            const stxxl::int64 new_capacity =
                disk_allocators[i]->new_blocks(disk_bids[i]);
            if (old_capacity != new_capacity)
            {
                // resize the file
                disk_files[i]->set_size(new_capacity);
                if (new_capacity != disk_allocators[i]->get_total_bytes())
                    STXXL_ERRMSG("File resizing failed: actual size " << disk_allocators[i]->get_total_bytes() << " != requested size " << new_capacity);
            }
        }
    }

    memset(bl, 0, ndisks * sizeof(int_type));

    OutputIterator it = out;
    for (i = 0; i != nblocks; ++it, ++i)
    {
        const int disk = disk_ptrs[i]->get_id();
        bid_type bid(disk_ptrs[i], disk_bids[disk][bl[disk]++].offset);
        *it = bid;
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:new    " << FMT_BID(bid));
    }

    delete[] bl;
    delete[] disk_bids;
    delete[] disk_ptrs;
}

template <class BlockType, class DiskAssignFunctor, class OutputIterator>
void block_manager::new_blocks(
    const unsigned_type nblocks,
    const DiskAssignFunctor & functor,
    OutputIterator out)
{
    typedef typename BlockType::bid_type bid_type;
    new_blocks_int<bid_type>(nblocks, functor, out);
}

template <class DiskAssignFunctor, class BIDIteratorClass>
void block_manager::new_blocks(
    const DiskAssignFunctor & functor,
    BIDIteratorClass bidbegin,
    BIDIteratorClass bidend)
{
    typedef typename std::iterator_traits<BIDIteratorClass>::value_type bid_type;

    unsigned_type nblocks = 0;

    BIDIteratorClass bidbegin_copy(bidbegin);
    while (bidbegin_copy != bidend)
    {
        ++bidbegin_copy;
        ++nblocks;
    }

    new_blocks_int<bid_type>(nblocks, functor, bidbegin);
}


template <unsigned BLK_SIZE>
void block_manager::delete_block(const BID<BLK_SIZE> & bid)
{
    // do not uncomment it
    //assert(bid.storage->get_id() < config::get_instance()->disks_number());
    if (bid.storage->get_id() == -1)
        return;  // self managed disk
    STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:delete " << FMT_BID(bid));
    assert(bid.storage->get_id() >= 0);
    disk_allocators[bid.storage->get_id()]->delete_block(bid);
    disk_files[bid.storage->get_id()]->delete_region(bid.offset, bid.size);
}


template <class BIDIteratorClass>
void block_manager::delete_blocks(
    const BIDIteratorClass & bidbegin,
    const BIDIteratorClass & bidend)
{
    for (BIDIteratorClass it = bidbegin; it != bidend; it++)
    {
        delete_block(*it);
    }
}

// in bytes
#ifndef STXXL_DEFAULT_BLOCK_SIZE
    #define STXXL_DEFAULT_BLOCK_SIZE(type) (2 * 1024 * 1024) // use traits
#endif

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_MNG_HEADER
// vim: et:ts=4:sw=4
