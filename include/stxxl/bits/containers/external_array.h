/***************************************************************************
 *  include/stxxl/bits/containers/ppq_external_array.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_PPQ_EXTERNAL_ARRAY_HEADER
#define STXXL_CONTAINERS_PPQ_EXTERNAL_ARRAY_HEADER

#include <cassert>
#include <ctime>
#include <iterator>
#include <utility>
#include <vector>

#include <stxxl/bits/io/request_operations.h>
#include <stxxl/bits/mng/block_alloc.h>
#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/bits/mng/read_write_pool.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/parallel.h>

STXXL_BEGIN_NAMESPACE

//! External array stores a sorted sequence of values on the hard disk and
//!		allows random access to the first block (containing the smallest values).
//!		The class uses buffering and prefetching in order to improve the performance.
//!
//! \tparam ValueType           Type of the contained objects (POD with no references to internal memory).
//!
//! \tparam BlockSize		External block size. Default = STXXL_DEFAULT_BLOCK_SIZE(ValueType).
//!
//! \tparam AllocStrategy	Allocation strategy for the external memory. Default = STXXL_DEFAULT_ALLOC_STRATEGY.
template <
    class ValueType,
    unsigned BlockSize = STXXL_DEFAULT_BLOCK_SIZE(ValueType),
    class AllocStrategy = STXXL_DEFAULT_ALLOC_STRATEGY
    >
class external_array // : private noncopyable
{
public:
    typedef external_array<ValueType, BlockSize, AllocStrategy> Self;
    typedef std::vector<BID<BlockSize> > bid_vector;
    typedef typed_block<BlockSize, ValueType> block_type;
    typedef ValueType* iterator;
    typedef const ValueType* const_iterator;
    typedef typename bid_vector::iterator bid_iterator;
    typedef typename block_type::iterator block_iterator;
    typedef read_write_pool<block_type> pool_type;

private:
    //! Allocation strategy for new blocks
    AllocStrategy alloc_strategy;

    //! The total number of elements. Cannot be changed after construction.
    size_t m_size;

    //! The total number of elements minus the number of extracted values
    size_t real_size;

    //! The size of one value in bytes
    size_t value_size;

    //! The number of elements fitting into one block
    size_t num_elements_per_block;

    //! Number of blocks in external memory
    size_t num_bids;

    //! Number of blocks to prefetch from HD
    size_t num_prefetch_blocks;

    //! Size of the write buffer in number of blocks
    size_t num_write_buffer_blocks;

    //! The IDs of each block in external memory
    bid_vector bids;

    //! The block with the currently smallest elements
    block_type* first_block;

    //! Prefetch and write buffer pool
    pool_type* pool;

    //! The read_request can be used to wait until the block has been completely fetched.
    request_ptr read_request;

    //! The write position in the block currently being filled. Used by the <<-operator.
    size_t write_position;

    //! Index of the next block to be prefetched.
    size_t hint_index;

    /* Write phase: Index of the external block where the current block will be stored in when it's filled.
     * Read phase: Index of the external block which will be fetched next.
     */
    size_t current_bid_index;

    //! True means write phase, false means read phase.
    bool write_phase;

    //! The first block is valid if wait_for_first_block() has already been called.
    bool _first_block_valid;

    //! Indicates if first_block is actually the first block of all which has never been written to external memory.
    bool is_first_block;

    //! boundaries of first block
    size_t begin_index;
    size_t end_index;

public:
    //! Constructs an external array
    //!
    //! \param size					The total number of elements. Cannot be changed after construction.
    //!
    //! \param _num_prefetch_blocks		Number of blocks to prefetch from hard disk
    //!
    //! \param _num_write_buffer_blocks	Size of the write buffer in number of blocks
    external_array(size_t size, size_t _num_prefetch_blocks, size_t _num_write_buffer_blocks)
        : m_size(size),
          real_size(0),
          value_size(sizeof(ValueType)),
          num_elements_per_block((size_t)(BlockSize / value_size)),
          num_bids((size_t)div_ceil(m_size, num_elements_per_block)),
          num_prefetch_blocks(std::min(_num_prefetch_blocks, num_bids)),
          num_write_buffer_blocks(std::min(_num_write_buffer_blocks, num_bids)),
          bids(num_bids),
          write_position(0),
          hint_index(0),
          current_bid_index(0),
          write_phase(true),
          _first_block_valid(false),
          is_first_block(true),
          begin_index(0),
          end_index(0)
    {
        assert(size > 0);
        first_block = new block_type;
        pool = new pool_type(0, num_write_buffer_blocks);
        block_manager* bm = block_manager::get_instance();
        bm->new_blocks(alloc_strategy, bids.begin(), bids.end());
    }

    //! Delete copy assignment for emplace_back to use the move semantics.
    Self& operator = (const Self& other) = delete;

    //! Delete copy constructor for emplace_back to use the move semantics.
    external_array(const Self& other) = delete;

    //! Move assignment.
    Self& operator = (Self&& o)
    {
        m_size = o.m_size;
        real_size = o.real_size;
        value_size = o.value_size;
        num_elements_per_block = o.num_elements_per_block;
        num_bids = o.num_bids;
        num_prefetch_blocks = o.num_prefetch_blocks;
        num_write_buffer_blocks = o.num_write_buffer_blocks;

        bids = std::move(o.bids);

        first_block = o.first_block;
        pool = o.pool;
        write_position = o.write_position;
        hint_index = o.hint_index;
        current_bid_index = o.current_bid_index;
        write_phase = o.write_phase;
        _first_block_valid = o._first_block_valid;
        is_first_block = o.is_first_block;
        begin_index = o.begin_index;
        end_index = o.end_index;

        o.first_block = nullptr;
        o.pool = nullptr;
        return *this;
    }

    //! Move constructor. Needed for regrowing in surrounding vector.
    external_array(Self&& o)
        : m_size(o.m_size),
          real_size(o.real_size),
          value_size(o.value_size),
          num_elements_per_block(o.num_elements_per_block),
          num_bids(o.num_bids),
          num_prefetch_blocks(o.num_prefetch_blocks),
          num_write_buffer_blocks(o.num_write_buffer_blocks),

          bids(std::move(o.bids)),

          first_block(o.first_block),
          pool(o.pool),
          write_position(o.write_position),
          hint_index(o.hint_index),
          current_bid_index(o.current_bid_index),
          write_phase(o.write_phase),
          _first_block_valid(o._first_block_valid),
          is_first_block(o.is_first_block),
          begin_index(o.begin_index),
          end_index(o.end_index)
    {
        o.first_block = nullptr;
        o.pool = nullptr;
    }

    //! Default constructor. Don't use this directy. Needed for regrowing in surrounding vector.
    external_array() = default;

    //! Destructor
    ~external_array()
    {
        block_manager* bm = block_manager::get_instance();
        if (pool != NULL) {
            // This could also be done step by step in remove_first_...() in future.
            bm->delete_blocks(bids.begin(), bids.end());
            delete first_block;
            delete pool;
        } // else: the array has been moved.
    }

    //! Returns the current size
    size_t size() const
    {
        return real_size;
    }

    //! Returns if the array is empty
    bool empty() const
    {
        return (real_size == 0);
    }

    //! Returns if one can safely access elements from the first block.
    bool first_block_valid() const
    {
        return _first_block_valid;
    }

    //! Returns the smallest element in the array
    ValueType & get_min_element()
    {
        assert(_first_block_valid);
        assert(end_index - 1 >= begin_index);
        return *begin_block();
    }

    //! Returns the largest element in the first block / internal memory
    ValueType & get_current_max_element()
    {
        assert(_first_block_valid);
        assert(end_index - 1 >= begin_index);
        block_iterator i = end_block();
        return *(--i);
    }

    //! Begin iterator of the first block
    iterator begin_block()
    {
        assert(_first_block_valid);
        return first_block->begin() + begin_index;
    }

    //! End iterator of the first block
    iterator end_block()
    {
        assert(_first_block_valid);
        return first_block->begin() + end_index;
    }

    //! Random access operator for the first block
    ValueType& operator [] (size_t i)
    {
        assert(i >= begin_index);
        assert(i < end_index);
        return first_block->elem[i];
    }

    //! Pushes <record> at the end of the array
    void operator << (const ValueType& record)
    {
        assert(write_phase);
        assert(real_size < m_size);
        ++real_size;

        if (UNLIKELY(write_position >= block_type::size)) {
            write_position = 0;
            write_out();
        }

        first_block->elem[write_position++] = record;
    }

    //! Finish write phase. Afterwards the values can be extracted from bottom up (ascending order).
    void finish_write_phase()
    {
        // Idea for the future: If we write the block with the smallest values in the end,
        // we don't need to write it into EM.

        assert(m_size == real_size);

        if (write_position > 0) {
            // This works only if the <<-operator has been used.
            write_out();
        }

        pool->resize_write(0);
        pool->resize_prefetch(num_prefetch_blocks);
        write_phase = false;

        current_bid_index = 0;
        begin_index = 0;
        end_index = write_position;

        for (size_t i = 0; i < num_prefetch_blocks; ++i) {
            hint();
        }

        load_next_block();
    }

    //! Removes the first block and loads the next one if there's any
    void remove_first_block()
    {
        assert(_first_block_valid);

        if (has_next_block()) {
            real_size -= num_elements_per_block;
            load_next_block();
        } else {
            real_size = 0;
            begin_index = 0;
            end_index = 0;
            _first_block_valid = true;
        }
    }

    //! Removes the first <n> elements from the array (first block).
    //! Loads the next block if the current one has run empty.
    //! Make shure there are at least <n> elements left in the first block.
    void remove_first_n_elements(size_t n)
    {
        assert(_first_block_valid);

        if (begin_index + n < end_index) {
            real_size -= n;
            begin_index += n;
        } else {
            assert(begin_index + n == end_index);

            if (has_next_block()) {
                assert(real_size >= n);
                real_size -= n;
                load_next_block();
            } else {
                real_size -= n;
                assert(real_size == 0);
                begin_index = 0;
                end_index = 0;
                _first_block_valid = true;
            }
        }
    }

    //! Has to be called before using begin_block() and end_block() in order to set _first_block_valid.
    void wait_for_first_block()
    {
        assert(!write_phase);

        if (_first_block_valid)
            return;

        if (read_request) {
            read_request->wait();
        }

        _first_block_valid = true;
    }

private:
    //! Write the current block into external memory
    void write_out()
    {
        assert(write_phase);
        pool->write(first_block, bids[current_bid_index++]);
        first_block = pool->steal();
    }

    //! Gives the prefetcher a hint for the next block to prefetch
    void hint()
    {
        if (hint_index < num_bids) {
            pool->hint(bids[hint_index]);
        }
        ++hint_index;
    }

    //! Fetches the next block from the hard disk and calls hint() afterwards.
    void load_next_block()
    {
        _first_block_valid = false;

        if (!is_first_block) {
            ++current_bid_index;
        } else {
            is_first_block = false;
        }

        read_request = pool->read(first_block, bids[current_bid_index]);
        hint();

        begin_index = 0;
        if (is_last_block()) {
            end_index = real_size % num_elements_per_block;
            end_index = (end_index == 0 ? num_elements_per_block : end_index);
        } else {
            end_index = num_elements_per_block;
        }
    }

    //! Returns if there is a next block on hard disk.
    bool has_next_block() const
    {
        return ((is_first_block && num_bids > 0) || (current_bid_index + 1 < num_bids));
    }

    //! Returns if the current block is the last one.
    bool is_last_block() const
    {
        return ((current_bid_index == num_bids - 1) || (m_size < num_elements_per_block));
    }
};

STXXL_END_NAMESPACE

#endif // !STXXL_CONTAINERS_PPQ_EXTERNAL_ARRAY_HEADER
