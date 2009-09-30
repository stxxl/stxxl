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
#include <stxxl/bits/common/rand.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/bits/singleton.h>
#include <stxxl/bits/mng/bid.h>
#include <stxxl/bits/mng/diskallocator.h>


__STXXL_BEGIN_NAMESPACE

//! \defgroup mnglayer Block management layer
//! Group of classes which help controlling external memory space,
//! managing disks, and allocating and deallocating blocks of external storage
//! \{

//! \brief Access point to disks properties
//! \remarks is a singleton
class config : public singleton<config>
{
    friend class singleton<config>;

    struct DiskEntry
    {
        std::string path;
        std::string io_impl;
        stxxl::int64 size;
        bool delete_on_exit;
    };
    std::vector<DiskEntry> disks_props;

    // in disks_props, flash devices come after all regular disks
    unsigned first_flash;

    config()
    {
        const char * cfg_path = getenv("STXXLCFG");
        if (cfg_path)
            init(cfg_path);
        else
            init();
    }

    ~config()
    {
        for (unsigned i = 0; i < disks_props.size(); ++i) {
            if (disks_props[i].delete_on_exit) {
                STXXL_ERRMSG("Removing disk file created from default configuration: " << disks_props[i].path);
                unlink(disks_props[i].path.c_str());
            }
        }
    }

    void init(const char * config_path = "./.stxxl");

public:
    //! \brief Returns number of disks available to user
    //! \return number of disks
    inline unsigned disks_number() const
    {
        return disks_props.size();
    }

    //! \brief Returns contiguous range of regular disks w/o flash devices in the array of all disks
    //! \return range [begin, end) of regular disk indices
    inline std::pair<unsigned, unsigned> regular_disk_range() const
    {
        return std::pair<unsigned, unsigned>(0, first_flash);
    }

    //! \brief Returns contiguous range of flash devices in the array of all disks
    //! \return range [begin, end) of flash device indices
    inline std::pair<unsigned, unsigned> flash_range() const
    {
        return std::pair<unsigned, unsigned>(first_flash, disks_props.size());
    }

    //! \brief Returns path of disks
    //! \param disk disk's identifier
    //! \return string that contains the disk's path name
    inline const std::string & disk_path(int disk) const
    {
        return disks_props[disk].path;
    }
    //! \brief Returns disk size
    //! \param disk disk's identifier
    //! \return disk size in bytes
    inline stxxl::int64 disk_size(int disk) const
    {
        return disks_props[disk].size;
    }
    //! \brief Returns name of I/O implementation of particular disk
    //! \param disk disk's identifier
    inline const std::string & disk_io_impl(int disk) const
    {
        return disks_props[disk].io_impl;
    }
};


class FileCreator
{
public:
    virtual stxxl::file * create(const std::string & io_impl,
                                 const std::string & filename,
                                 int options, int disk);

    virtual ~FileCreator() { }
};

//! \weakgroup alloc Allocation functors
//! Standard allocation strategies encapsulated in functors
//! \{

//! \brief example disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct basic_allocation_strategy
{
    basic_allocation_strategy(int disks_begin, int disks_end);
    basic_allocation_strategy();
    int operator () (int i) const;
    static const char * name();
};

//! \brief striping disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct striping
{
    int begin, diff;
    striping(int b, int e) : begin(b), diff(e - b)
    { }
    striping() : begin(0)
    {
        diff = config::get_instance()->disks_number();
    }
    int operator () (int i) const
    {
        return begin + i % diff;
    }
    static const char * name()
    {
        return "striping";
    }
    // FIXME WHY?
    virtual ~striping()
    { }
};

//! \brief fully randomized disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct FR : public striping
{
    random_number<random_uniform_fast> rnd;
    FR(int b, int e) : striping(b, e)
    { }
    FR() : striping()
    { }
    int operator () (int /*i*/) const
    {
        return begin + rnd(diff);
    }
    static const char * name()
    {
        return "fully randomized striping";
    }
};

//! \brief simple randomized disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct SR : public striping
{
    random_number<random_uniform_fast> rnd;
    int offset;
    SR(int b, int e) : striping(b, e)
    {
        offset = rnd(diff);
    }
    SR() : striping()
    {
        offset = rnd(diff);
    }
    int operator () (int i) const
    {
        return begin + (i + offset) % diff;
    }
    static const char * name()
    {
        return "simple randomized striping";
    }
};

//! \brief randomized cycling disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct RC : public striping
{
    std::vector<int> perm;

    RC(int b, int e) : striping(b, e), perm(diff)
    {
        for (int i = 0; i < diff; i++)
            perm[i] = i;

        stxxl::random_number<random_uniform_fast> rnd;
        std::random_shuffle(perm.begin(), perm.end(), rnd __STXXL_FORCE_SEQUENTIAL);
    }
    RC() : striping(), perm(diff)
    {
        for (int i = 0; i < diff; i++)
            perm[i] = i;

        random_number<random_uniform_fast> rnd;
        std::random_shuffle(perm.begin(), perm.end(), rnd __STXXL_FORCE_SEQUENTIAL);
    }
    int operator () (int i) const
    {
        return begin + perm[i % diff];
    }
    static const char * name()
    {
        return "randomized cycling striping";
    }
};

struct RC_disk : public RC
{
    RC_disk(int b, int e) : RC(b, e)
    { }
    RC_disk() : RC(config::get_instance()->regular_disk_range().first, config::get_instance()->regular_disk_range().second)
    { }
    static const char * name()
    {
        return "Randomized cycling striping on regular disks";
    }
};

struct RC_flash : public RC
{
    RC_flash(int b, int e) : RC(b, e)
    { }
    RC_flash() : RC(config::get_instance()->flash_range().first, config::get_instance()->flash_range().second)
    { }
    static const char * name()
    {
        return "Randomized cycling striping on flash devices";
    }
};

//! \brief 'single disk' disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct single_disk
{
    const int disk;
    single_disk(int d, int = 0) : disk(d)
    { }

    single_disk() : disk(0)
    { }
    int operator () (int /*i*/) const
    {
        return disk;
    }
    static const char * name()
    {
        return "single disk";
    }
};

//! \brief Allocator functor adaptor

//! Gives offset to disk number sequence defined in constructor
template <class BaseAllocator_>
struct offset_allocator
{
    BaseAllocator_ base;
    int_type offset;

    //! \brief Creates functor based on instance of \c BaseAllocator_ functor
    //! with offset \c offset_
    //! \param offset_ offset
    //! \param base_ used to create a copy
    offset_allocator(int_type offset_, const BaseAllocator_ & base_) : base(base_), offset(offset_) { }
    int operator () (int_type i) const
    {
        return base(offset + i);
    }
};

//! \}

#if 0 // deprecated

//! \brief Traits for models of \b bid_iterator concept
template <class bid_it>
struct bid_iterator_traits
{
    bid_it * a;
    enum
    {
        block_size = bid_it::block_size
    };
};

template <unsigned blk_sz>
struct bid_iterator_traits<BID<blk_sz> *>
{
    enum
    {
        block_size = blk_sz
    };
};


template <unsigned blk_sz, class X>
struct bid_iterator_traits<__gnu_cxx::__normal_iterator<BID<blk_sz> *, X> >
{
    enum
    {
        block_size = blk_sz
    };
};

template <unsigned blk_sz, class X, class Y>
struct bid_iterator_traits<std::_List_iterator<BID<blk_sz>, X, Y> >
{
    enum
    {
        block_size = blk_sz
    };
};

template <unsigned blk_sz, class X>
struct bid_iterator_traits<typename std::vector<BID<blk_sz>, X>::iterator>
{
    enum
    {
        block_size = blk_sz
    };
};

#endif

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
        DiskAssignFunctor functor,
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
        DiskAssignFunctor functor,
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
        DiskAssignFunctor functor,
        BIDIteratorClass out);

    //! Allocates a new block according to the strategy
    //! given by \b functor and stores the block identifier
    //! to bid.
    //! \param functor object of model of \b allocation_strategy concept
    //! \param bid BID to store the block identifier
    template <typename DiskAssignFunctor, unsigned BLK_SIZE>
    void new_block(DiskAssignFunctor functor, BID<BLK_SIZE> & bid)
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
    DiskAssignFunctor functor,
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
    DiskAssignFunctor functor,
    OutputIterator out)
{
    typedef typename BlockType::bid_type bid_type;
    new_blocks_int<bid_type>(nblocks, functor, out);
}

template <class DiskAssignFunctor, class BIDIteratorClass>
void block_manager::new_blocks(
    DiskAssignFunctor functor,
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

#ifndef STXXL_DEFAULT_ALLOC_STRATEGY
    #define STXXL_DEFAULT_ALLOC_STRATEGY stxxl::RC
#endif

// in bytes
#ifndef STXXL_DEFAULT_BLOCK_SIZE
    #define STXXL_DEFAULT_BLOCK_SIZE(type) (2 * 1024 * 1024) // use traits
#endif

//! \}

__STXXL_END_NAMESPACE


#endif // !STXXL_MNG_HEADER
// vim: et:ts=4:sw=4
