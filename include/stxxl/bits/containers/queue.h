/***************************************************************************
 *  include/stxxl/bits/containers/queue.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_QUEUE_HEADER
#define STXXL_QUEUE_HEADER

#include <vector>
#include <queue>
#include <deque>

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/common/tmeta.h>
#include <stxxl/bits/mng/read_write_pool.h>
#include <stxxl/bits/mng/write_pool.h>
#include <stxxl/bits/mng/prefetch_pool.h>


__STXXL_BEGIN_NAMESPACE

#ifndef STXXL_VERBOSE_QUEUE
#define STXXL_VERBOSE_QUEUE STXXL_VERBOSE2
#endif

//! \addtogroup stlcont
//! \{


//! \brief External FIFO queue container

//! Template parameters:
//! - ValTp type of the contained objects
//! - BlkSz size of the external memory block in bytes, default is \c STXXL_DEFAULT_BLOCK_SIZE(ValTp)
//! - AllocStr parallel disk allocation strategy, default is \c STXXL_DEFAULT_ALLOC_STRATEGY
//! - SzTp size data type, default is \c stxxl::uint64
template <class ValTp,
          unsigned BlkSz = STXXL_DEFAULT_BLOCK_SIZE(ValTp),
          class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY,
          class SzTp = stxxl::uint64>
class queue : private noncopyable
{
public:
    typedef ValTp value_type;
    typedef AllocStr alloc_strategy_type;
    typedef SzTp size_type;
    enum {
        block_size = BlkSz
    };

    typedef typed_block<block_size, value_type> block_type;
    typedef BID<block_size> bid_type;

private:
    typedef offset_allocator<alloc_strategy_type> offset_alloc_strategy_type;
    typedef read_write_pool<block_type> pool_type;

    size_type size_;
    bool delete_pool;
    pool_type * pool;
    block_type * front_block;
    block_type * back_block;
    value_type * front_element;
    value_type * back_element;
    offset_alloc_strategy_type alloc_strategy;
    std::deque<bid_type> bids;
    block_manager * bm;
    unsigned_type blocks2prefetch;

public:
    //! \brief Constructs empty queue with own write and prefetch block pool

    //! \param w_pool_size  number of blocks in the write pool, must be at least 2, recommended at least 3
    //! \param p_pool_size  number of blocks in the prefetch pool, recommended at least 1
    //! \param blocks2prefetch_  defines the number of blocks to prefetch (\c front side),
    //!                          default is number of block in the prefetch pool
    explicit queue(unsigned_type w_pool_size = 3, unsigned_type p_pool_size = 1, int blocks2prefetch_ = -1) :
        size_(0),
        delete_pool(true),
        bm(block_manager::get_instance())
    {
        STXXL_VERBOSE_QUEUE("queue[" << this << "]::queue(sizes)");
        pool = new pool_type(p_pool_size, w_pool_size);
        init(blocks2prefetch_);
    }

    //! \brief Constructs empty queue

    //! \param w_pool write pool
    //! \param p_pool prefetch pool
    //! \param blocks2prefetch_  defines the number of blocks to prefetch (\c front side),
    //!                          default is number of blocks in the prefetch pool
    //!  \warning Number of blocks in the write pool must be at least 2, recommended at least 3
    //!  \warning Number of blocks in the prefetch pool recommended at least 1
    __STXXL_DEPRECATED(
    queue(write_pool<block_type> & w_pool, prefetch_pool<block_type> & p_pool, int blocks2prefetch_ = -1)) :
        size_(0),
        delete_pool(true),
        bm(block_manager::get_instance())
    {
        STXXL_VERBOSE_QUEUE("queue[" << this << "]::queue(pools)");
        pool = new pool_type(w_pool, p_pool);
        init(blocks2prefetch_);
    }

    //! \brief Constructs empty queue

    //! \param pool_ block write/prefetch pool
    //! \param blocks2prefetch_  defines the number of blocks to prefetch (\c front side),
    //!                          default is number of blocks in the prefetch pool
    //!  \warning Number of blocks in the write pool must be at least 2, recommended at least 3
    //!  \warning Number of blocks in the prefetch pool recommended at least 1
    queue(pool_type & pool_, int blocks2prefetch_ = -1) :
        size_(0),
        delete_pool(false),
        pool(&pool_),
        bm(block_manager::get_instance())
    {
        STXXL_VERBOSE_QUEUE("queue[" << this << "]::queue(pool)");
        init(blocks2prefetch_);
    }

private:
    void init(int blocks2prefetch_)
    {
        if (pool->size_write() < 2) {
            STXXL_ERRMSG("queue: invalid configuration, not enough blocks (" << pool->size_write() 
                         << ") in write pool, at least 2 are needed, resizing to 3");
            pool->resize_write(3);
        }

        if (pool->size_write() < 3) {
            STXXL_MSG("queue: inefficient configuration, no blocks for buffered writing available");
        }

        if (pool->size_prefetch() < 1) {
            STXXL_MSG("queue: inefficient configuration, no blocks for prefetching available");
        }

        front_block = back_block = pool->steal();
        back_element = back_block->elem - 1;
        front_element = back_block->elem;
        set_prefetch_aggr(blocks2prefetch_);
    }

public:
    //! \brief Defines the number of blocks to prefetch (\c front side)
    //!        This method should be called whenever the prefetch pool is resized
    //! \param blocks2prefetch_  defines the number of blocks to prefetch (\c front side),
    //!                          a negative value means to use the number of blocks in the prefetch pool
    void set_prefetch_aggr(int_type blocks2prefetch_)
    {
        if (blocks2prefetch_ < 0)
            blocks2prefetch = pool->size_prefetch();
        else
            blocks2prefetch = blocks2prefetch_;
    }

    //! \brief Returns the number of blocks prefetched from the \c front side
    unsigned_type get_prefetch_aggr() const
    {
        return blocks2prefetch;
    }

    //! \brief Adds an element in the queue
    void push(const value_type & val)
    {
        if (back_element == back_block->elem + (block_type::size - 1))
        {
            // back block is filled
            if (front_block == back_block)
            {             // can not write the back block because it
                // is the same as the front block, must keep it memory
                STXXL_VERBOSE1("queue::push Case 1");
            }
            else
            {
                STXXL_VERBOSE1("queue::push Case 2");
                // write the back block
                // need to allocate new block
                bid_type newbid;

                bm->new_block(alloc_strategy, newbid);
                alloc_strategy.set_offset(alloc_strategy.get_offset() + 1);

                STXXL_VERBOSE_QUEUE("queue[" << this << "]: push block " << back_block << " @ " << FMT_BID(newbid));
                bids.push_back(newbid);
                pool->write(back_block, newbid);
                if (bids.size() <= blocks2prefetch) {
                    STXXL_VERBOSE1("queue::push Case Hints");
                    pool->hint(newbid);
                }
            }
            back_block = pool->steal();

            back_element = back_block->elem;
            *back_element = val;
            ++size_;
            return;
        }
        ++back_element;
        *back_element = val;
        ++size_;
    }

    //! \brief Removes element from the queue
    void pop()
    {
        assert(!empty());

        if (front_element == front_block->elem + (block_type::size - 1))
        {
            // if there is only one block, it implies ...
            if (back_block == front_block)
            {
                STXXL_VERBOSE1("queue::pop Case 3");
                assert(size() == 1);
                assert(back_element == front_element);
                assert(bids.empty());
                // reset everything
                back_element = back_block->elem - 1;
                front_element = back_block->elem;
                size_ = 0;
                return;
            }

            --size_;
            if (size_ <= block_type::size)
            {
                STXXL_VERBOSE1("queue::pop Case 4");
                assert(bids.empty());
                // the back_block is the next block
                pool->add(front_block);
                front_block = back_block;
                front_element = back_block->elem;
                return;
            }
            STXXL_VERBOSE1("queue::pop Case 5");

            assert(!bids.empty());
            request_ptr req = pool->read(front_block, bids.front());
            STXXL_VERBOSE_QUEUE("queue[" << this << "]: pop block  " << front_block << " @ " << FMT_BID(bids.front()));

            // give prefetching hints
            for (unsigned_type i = 0; i < blocks2prefetch && i < bids.size() - 1; ++i)
            {
                STXXL_VERBOSE1("queue::pop Case Hints");
                pool->hint(bids[i + 1]);
            }

            front_element = front_block->elem;
            req->wait();

            bm->delete_block(bids.front());
            bids.pop_front();
            return;
        }

        ++front_element;
        --size_;
    }

    //! \brief Returns the size of the queue
    size_type size() const
    {
        return size_;
    }

    //! \brief Returns \c true if queue is empty
    bool empty() const
    {
        return (size_ == 0);
    }

    //! \brief Returns a mutable reference at the back of the queue
    value_type & back()
    {
        assert(!empty());
        return *back_element;
    }

    //! \brief Returns a const reference at the back of the queue
    const value_type & back() const
    {
        assert(!empty());
        return *back_element;
    }

    //! \brief Returns a mutable reference at the front of the queue
    value_type & front()
    {
        assert(!empty());
        return *front_element;
    }

    //! \brief Returns a const reference at the front of the queue
    const value_type & front() const
    {
        assert(!empty());
        return *front_element;
    }

    ~queue()
    {
        pool->add(front_block);
        if (front_block != back_block)
            pool->add(back_block);


        if (delete_pool)
        {
            delete pool;
        }

        if (!bids.empty())
            bm->delete_blocks(bids.begin(), bids.end());
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_QUEUE_HEADER
// vim: et:ts=4:sw=4
