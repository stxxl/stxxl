/***************************************************************************
 *  include/stxxl/bits/mng/write_pool.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_WRITE_POOL_HEADER
#define STXXL_WRITE_POOL_HEADER

#include <list>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <stxxl/bits/mng/mng.h>
//#include <stxxl/bits/compat_hash_map.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup schedlayer
//! \{


//! \brief Implements dynamically resizable buffered writing pool
template <class BlockType>
class write_pool : private noncopyable
{
public:
    typedef BlockType block_type;
    typedef typename block_type::bid_type bid_type;

    // a hack to make wait_any work with busy_entry type
    struct busy_entry
    {
        block_type * block;
        request_ptr req;
        bid_type bid;

        busy_entry() : block(NULL) { }
        busy_entry(const busy_entry & a) : block(a.block), req(a.req), bid(a.bid) { }
        busy_entry(block_type * & bl, request_ptr & r, bid_type & bi) :
            block(bl), req(r), bid(bi) { }

        operator request_ptr () { return req; }
    };
    //typedef typename compat_hash_map < bid_type, request_ptr , bid_hash >::result hash_map_type;
    //typedef typename hash_map_type::iterator block_track_iterator;
    typedef typename std::list<block_type *>::iterator free_blocks_iterator;
    typedef typename std::list<busy_entry>::iterator busy_blocks_iterator;

protected:
    // contains free write blocks
    std::list<block_type *> free_blocks;
    // blocks that are in writing
    std::list<busy_entry> busy_blocks;

    //hash_map_type block_track;

    unsigned_type free_blocks_size, busy_blocks_size;

public:
    //! \brief Constructs pool
    //! \param init_size initial number of blocks in the pool
    explicit write_pool(unsigned_type init_size = 1) : free_blocks_size(init_size), busy_blocks_size(0)
    {
        unsigned_type i = 0;
        for ( ; i < init_size; ++i)
            free_blocks.push_back(new block_type);
    }

    void swap(write_pool & obj)
    {
        std::swap(free_blocks, obj.free_blocks);
        std::swap(busy_blocks, obj.busy_blocks);
        std::swap(free_blocks_size, obj.free_blocks_size);
        std::swap(busy_blocks_size, busy_blocks_size);
    }

    //! \brief Waits for completion of all ongoing write requests and frees memory
    virtual ~write_pool()
    {
        STXXL_VERBOSE2("write_pool::~write_pool free_blocks_size: " <<
                       free_blocks_size << " busy_blocks_size: " << busy_blocks_size);
        while (!free_blocks.empty())
        {
            delete free_blocks.back();
            free_blocks.pop_back();
        }

        try
        {
            for (busy_blocks_iterator i2 = busy_blocks.begin(); i2 != busy_blocks.end(); ++i2)
            {
                i2->req->wait();
                delete i2->block;
            }
        }
        catch (...)
        { }
    }

    //! \brief Returns number of owned blocks
    unsigned_type size() const { return free_blocks_size + busy_blocks_size; }

    //! \brief Passes a block to the pool for writing
    //! \param block block to write. Ownership of the block goes to the pool.
    //! \c block must be allocated dynamically with using \c new .
    //! \param bid location, where to write
    //! \warning \c block must be allocated dynamically with using \c new .
    //! \return request object of the write operation
    request_ptr write(block_type * block, bid_type bid)
    {
        for (busy_blocks_iterator i2 = busy_blocks.begin(); i2 != busy_blocks.end(); ++i2)
        {
            if (i2->bid == bid && i2->block != block) {
                STXXL_VERBOSE1("WAW dependency");
            }
        }
        request_ptr result = block->write(bid);
        ++busy_blocks_size;
        busy_blocks.push_back(busy_entry(block, result, bid));
        return result;
    }

    //! \brief Take out a block from the pool
    //! \return pointer to the block. Ownership of the block goes to the caller.
    block_type * steal()
    {
        assert(size() > 0);
        if (free_blocks_size)
        {
            STXXL_VERBOSE1("write_pool::steal : " << free_blocks_size << " free blocks available");
            --free_blocks_size;
            block_type * p = free_blocks.back();
            free_blocks.pop_back();
            return p;
        }
        STXXL_VERBOSE1("write_pool::steal : all " << busy_blocks_size << " are busy");
        busy_blocks_iterator completed = wait_any(busy_blocks.begin(), busy_blocks.end());
        assert(completed != busy_blocks.end()); // we got something reasonable from wait_any
        assert(completed->req->poll());         // and it is *really* completed
        block_type * p = completed->block;
        busy_blocks.erase(completed);
        --busy_blocks_size;
        check_all_busy();                       // for debug
        return p;
    }

    // deprecated name for the steal()
    block_type * get()
    {
        return steal();
    }

    //! \brief Resizes size of the pool
    //! \param new_size new size of the pool after the call
    void resize(unsigned_type new_size)
    {
        int_type diff = int_type(new_size) - int_type(size());
        if (diff > 0)
        {
            free_blocks_size += diff;
            while (--diff >= 0)
                free_blocks.push_back(new block_type);


            return;
        }

        while (++diff <= 0)
            delete get();
    }

    request_ptr get_request(bid_type bid)
    {
        busy_blocks_iterator i2 = busy_blocks.begin();
        for ( ; i2 != busy_blocks.end(); ++i2)
        {
            if (i2->bid == bid)
                return i2->req;
        }
        return request_ptr();
    }


    block_type * steal(bid_type bid)
    {
        busy_blocks_iterator i2 = busy_blocks.begin();
        for ( ; i2 != busy_blocks.end(); ++i2)
        {
            if (i2->bid == bid)
            {
                block_type * p = i2->block;
                i2->req->wait();
                busy_blocks.erase(i2);
                --busy_blocks_size;
                return p;
            }
        }
        return NULL;
    }

    void add(block_type * block)
    {
        free_blocks.push_back(block);
        ++free_blocks_size;
    }

protected:
    void check_all_busy()
    {
        busy_blocks_iterator cur = busy_blocks.begin();
        int_type cnt = 0;
#if STXXL_VERBOSE_LEVEL > 0
        int_type busy_blocks_size_old = busy_blocks_size;
#endif
        for ( ; cur != busy_blocks.end(); ++cur)
        {
            if (cur->req->poll())
            {
                free_blocks.push_back(cur->block);
                busy_blocks.erase(cur);
                cur = busy_blocks.begin();
                ++cnt;
                --busy_blocks_size;
                ++free_blocks_size;
                if (busy_blocks.empty())
                    break;
            }
        }
        STXXL_VERBOSE1("write_pool::check_all_busy : " << cnt <<
                       " are completed out of " << busy_blocks_size_old << " busy blocks");
    }
};

//! \}

__STXXL_END_NAMESPACE


namespace std
{
    template <class BlockType>
    void swap(stxxl::write_pool<BlockType> & a,
              stxxl::write_pool<BlockType> & b)
    {
        a.swap(b);
    }
}

#endif // !STXXL_WRITE_POOL_HEADER
