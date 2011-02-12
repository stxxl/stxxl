/***************************************************************************
 *  include/stxxl/bits/mng/block_scheduler.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_BLOCK_SCHEDULER_HEADER
#define STXXL_BLOCK_SCHEDULER_HEADER

#include <queue>
#include <stack>
#include <set>

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>

__STXXL_BEGIN_NAMESPACE

template <typename ElementType>
class priority_adressable_p_queue
{
protected:
    std::set<ElementType> vals;

public:
    bool empty() const
    { return vals.empty(); }

    void insert(const ElementType & e)
    { vals.insert(e); }

    void remove(const ElementType & e)
    { vals.erase(e); }

    ElementType pop_min()
    {
        const ElementType e = * vals.begin();
        vals.erase(vals.begin());
        return e;
    }

    ElementType pop_max()
    {
        const ElementType e = * vals.rbegin();
        vals.erase(vals.rbegin());
        return e;
    }
};

//! \brief Holds information to allocate, swap and reload a block of data. Use with block_scheduler.
//! A swapable_block can be uninitialized, i.e. it holds no data.
//! When access is required, is has to be acquired and released afterwards, so it can be swapped in and out as required.
//! If the stored data is no longer needed, it can get uninitialized, freeing in- and external memory.
//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam BlockSize number of contained objects
template <typename ValueType, unsigned BlockSize>
class swapable_block : private noncopyable
{
protected:
    static const unsigned_type raw_block_size = BlockSize * sizeof(ValueType);
public:
    typedef typed_block<raw_block_size, ValueType> internal_block_type;
    typedef typename internal_block_type::bid_type external_block_type;

protected:
    template <class SBT> friend class block_scheduler;

    external_block_type external_data;      //!external_data.valid if no associated space on disk
    internal_block_type * internal_data;    //NULL if there is no internal memory reserved
    int_type reference_count;
    bool dirty;

public:
    //! \brief create in uninitialized state
    swapable_block()
        : external_data() /*!valid*/, internal_data(0), reference_count(0), dirty(false) {}

    //! \brief create in initialized, swapped out state
    //! \param bid external_block with data, will be used as swap-space of this swapable_block
    swapable_block(const external_block_type & bid)
        : external_data(bid), internal_data(0), reference_count(0), dirty(false) {}

    ~swapable_block()
    {
        if (internal_data)
        {
            STXXL_ERRMSG("Destructing swapable_block that still holds internal_block.");
            //todo delete?
        }
        if (reference_count > 0)
            STXXL_ERRMSG("Destructing swapable_block that is still referenced.");
        if (dirty)
            STXXL_ERRMSG("Destructing swapable_block that is still dirty.");
    }

protected:
    bool is_internal() const
    { return internal_data; }

    bool is_external() const
    { return external_data.valid(); }

    bool is_acquired() const
    { return reference_count > 0; }

public:
    bool is_initialized() const
    { return is_internal() || is_external(); }

    internal_block_type & get_internal_block()
    {
        assert(is_acquired());
        return * internal_data;
    }

    //! \brief fill block with default data, is supposed to be overwritten by subclass
    void fill_default() {}

    //! \brief fill block with specific value
    void fill(const ValueType & value)
    {
        // get_internal_block checks acquired
        internal_block_type & data = get_internal_block();
        for (int_type i = 0; i < BlockSize; ++i)
            data[i] = value;
    }
};

enum block_scheduler_operation
{
    acquire,
    release,
    release_tainted,
    deinitialize,
    explicit_timestep
};

template <class SwapableBlockType>
struct swapable_block_identifier
{
    typedef int_type temporary_swapable_blocks_index_type;

    SwapableBlockType * persistent;
    temporary_swapable_blocks_index_type temporary;
};

//a prediction sequence is a sequence of require and release operations
template <class SwapableBlockType>
class prediction_sequence_element
{
protected:
    typedef swapable_block_identifier<SwapableBlockType> swapable_block_identifier_type;
    typedef typename swapable_block_identifier_type::temporary_swapable_blocks_index_type temporary_swapable_blocks_index_type;

    template <class SBT>
    friend class block_scheduler;

    block_scheduler_operation op;
    swapable_block_identifier_type id;

    prediction_sequence_element(block_scheduler_operation operation, temporary_swapable_blocks_index_type  temporary_swapable_block)
        : op(operation), id(0,temporary_swapable_block) { }

public:
    prediction_sequence_element(block_scheduler_operation operation, SwapableBlockType *  swapable_block)
        : op(operation), id(swapable_block,0) { }
};

//! \brief Swaps swapable_blocks and provides swapable_blocks for temporary storage.
//! Simple mode only tries to save I/Os through caching.
//! Features a simulation mode to record access patterns in a prediction sequence.
//!   The prediction sequence can then be used for prefetching during a run in offline mode.
//!   This will only work for algorithms with deterministic, data oblivious access patterns.
//!   In simulation mode no I/O is performed; the data provided is accessible but undefined.
//! Execute mode does caching, prefetching and possibly other optimizations.
//! \tparam SwapableBlockType Type of swapable_blocks to manage. Can be some specialized subclass.
template <class SwapableBlockType>
class block_scheduler : private noncopyable
{
protected:
    // tuning-parameter: To acquire blocks, internal memory has to be allocated.
    // This constant limits the number of internal_blocks allocated at once.
    static const int_type max_internal_blocks_alloc_at_once = 128;

    //! Mode the block scheduler currently works in
    enum Mode
    {
        online,         //serve requests immediately, without any prediction, LRU caching
        simulation,     //record prediction sequence only, do not serve requests, (returned blocks must not be accessed)
        offline_lfd,    //serve requests based on prediction sequence, using longest-forward-distance caching
        //offline_lfd_prefetch     //serve requests based on prediction sequence, using longest-forward-distance caching, and prefetching
    };

    typedef typename SwapableBlockType::internal_block_type internal_block_type;
    typedef unsigned_type evictable_block_priority_type;
    typedef std::pair<evictable_block_priority_type, SwapableBlockType *> evictable_block_priority_element_type;
    typedef typename prediction_sequence_element<SwapableBlockType>::temporary_swapable_blocks_index_type temporary_swapable_blocks_index_type;
public:
    typedef std::vector< prediction_sequence_element<SwapableBlockType> > prediction_sequence_type;

protected:
    const int_type max_internal_blocks;
    int_type remaining_internal_blocks;
    //! \brief holds free internal_blocks with attributes reset
    std::stack<internal_block_type * > free_internal_blocks;
    //! \brief temporary blocks that will not be needed after algorithm termination
    std::vector<SwapableBlockType> temporary_swapable_blocks;
    //! \brief holds indices of free temporary swapable_blocks with attributes reset
    std::stack<temporary_swapable_blocks_index_type> free_temporary_swapable_blocks;
    prediction_sequence_type prediction_sequence;
    //! \brief holds swapable blocks, whose internal block can be freed, i.e. that are internal but unacquired
    priority_adressable_p_queue<evictable_block_priority_element_type> evictable_blocks;
    Mode mode;
    block_manager * bm;

public:
    //! \brief create a block_scheduler with empty prediction sequence in simple mode.
    //! \param max_internal_memory Amount of internal memory (in bytes) the scheduler is allowed to use for acquiring, prefetching and caching.
    explicit block_scheduler(int_type max_internal_memory)
        : max_internal_blocks(div_ceil(max_internal_memory, sizeof(internal_block_type))),
          remaining_internal_blocks(max_internal_blocks),
          mode(online),
          bm(block_manager::get_instance())
    {
        //todo
    }

protected:
    internal_block_type * get_free_internal_block()
    {
        if (! free_internal_blocks.empty())
        {
            // => there are internal_blocks in the free-list
            internal_block_type * iblock = free_internal_blocks.top();
            free_internal_blocks.pop();
            return iblock;
        }
        else if (remaining_internal_blocks > 0)
        {
            // => more internal_blocks can be allocated
            internal_block_type * iblocks[max_internal_blocks_alloc_at_once];
            int_type i = 0; // invariant: i == number of newly allocated blocks not in the free-list
            // allocate up to max_internal_blocks_alloc_at_once, but do not allocate anything else in between
            while (i < max_internal_blocks_alloc_at_once && remaining_internal_blocks > 0)
            {
                --remaining_internal_blocks;
                iblocks[i] = new internal_block_type;
                ++i;
            }
            // put all but one block in the free-list (may cause allocation of list-space)
            while (i > 1)
                free_internal_blocks.push(iblocks[--i]);
            // i == 1
            return iblocks[0];
        }
        else
        {
            //todo
            //free block from pq intern_swapable_blocks
            //swap out
            //reset block
            return 0;
        }
    }

    temporary_swapable_blocks_index_type temporary_swapable_blocks_index(const SwapableBlockType & sblock) const
    {
        assert(& sblock >= temporary_swapable_blocks.begin() && & sblock < temporary_swapable_blocks.end());
        return & sblock - temporary_swapable_blocks.begin();
    }

    evictable_block_priority_type get_block_priority(const SwapableBlockType & /*sblock*/) const
    {
        //todo
        return 0;
    }

public:
    //! \brief Acquire the given block.
    //! Has to be in pairs with release. Pairs may be nested and interleaved.
    //! \return Reference to the block's data.
    //! \param sblock Swapable block to acquire.
    internal_block_type & acquire(SwapableBlockType & sblock)
    {
        switch (mode) {
        case online:
            // acquired => intern -> increase reference count
            // intern but not acquired -> remove from evictable_blocks, increase reference count
            // not intern => uninitialized or extern -> get internal_block, increase reference count
            // uninitialized -> fill with default value
            // extern -> read
            if (sblock.is_internal())
            {
                if (! sblock.is_acquired())
                    // not acquired yet -> remove from pq
                    evictable_blocks.remove(evictable_block_priority_element_type(get_block_priority(sblock) ,& sblock));
            }
            else if (sblock.is_initialized())
            {
                // => external but not internal
                //get internal_block
                sblock.internal_data = get_free_internal_block();
                //load block synchronously
                sblock.internal_data->read(sblock.external_data) ->wait();
            }
            else
            {
                // => ! sblock.is_initialized()
                //get internal_block
                sblock.internal_data = get_free_internal_block();
                //initialize new block
                sblock.fill_default();
            }
            //increase reference count
            ++ sblock.reference_count;
            break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }
        return sblock.get_internal_block();
    }

    //! \brief Release the given block.
    //! Has to be in pairs with acquire. Pairs may be nested and interleaved.
    //! \param sblock Swapable block to release.
    //! \param dirty If the data hab been changed, invalidating possible data in external storage.
    void release(SwapableBlockType & sblock, bool dirty)
    {
        switch (mode) {
        case online:
            if (sblock.is_acquired())
            {
                sblock.dirty |= dirty;
                -- sblock.reference_count;
                if (! sblock.is_acquired())
                {
                    // dirty or external -> evictable, put in pq
                    if (sblock.dirty || sblock.is_external())
                        evictable_blocks.insert(evictable_block_priority_element_type(get_block_priority(sblock) ,& sblock));
                    //else -> uninitialized, release internal block and put it in freelist
                    else
                    {
                        free_internal_blocks.push(sblock.internal_data);
                        sblock.internal_data = 0;
                    }
                }
            }
            else
            {
                //todo
                //error
            }
            break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }
    }

    //! \brief Drop all data in the given block, freeing in- and external memory.
    void deinitialize(SwapableBlockType & sblock)
    {
        switch (mode) {
        case online:
            if (! sblock.is_acquired())
            {
                // reset and free internal_block
                if (sblock.is_internal())
                {
                    sblock.internal_data->reset();
                    free_internal_blocks.push(sblock.internal_data);
                    sblock.internal_data = 0;
                }
                // free external_block (so that it becomes invalid and the disk-space can be used again)
                if (sblock.is_extern())
                {
                    bm->delete_block(sblock.external_data);
                }
            }
            else
            {
                //todo
                //error
            }
            break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }
    }

    //! \brief Record a timestep in the prediction sequence to seperate consecutive acquire rsp. release-operations.
    //! Has an effect only in simulation mode.
    void explicit_timestep()
    {
        switch (mode) {
        case online:
            break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }
    }

    //! \brief Get a const reference to given block's data. Block has to be already acquired.
    //! \param sblock Swapable block to access.
    const internal_block_type & get_internal_block(const SwapableBlockType & sblock) const
    { return sblock.get_internal_block(); }

    //! \brief Get a reference to given block's data. Block has to be already acquired.
    //! \param sblock Swapable block to access.
    internal_block_type & get_internal_block(SwapableBlockType & sblock) const
    { return sblock.get_internal_block(); }

    //! \brief Allocate a temporary swapable_block. Returns a pointer to the block.
    //! Temporary blocks are considered in scheduling. They may never be written to external memory if there is enough main memory.
    SwapableBlockType * allocate_temporary_swapable_block()
    {
        switch (mode) {
        case online:
            SwapableBlockType * sblock;
            if (free_temporary_swapable_blocks.empty())
            {
                // create new temporary swapable_block
                temporary_swapable_blocks.pushback();
                sblock = temporary_swapable_blocks.back();
            }
            else
            {
                // take swapable_block from freelist
                sblock = & temporary_swapable_blocks[free_temporary_swapable_blocks.top()];
                free_temporary_swapable_blocks.pop();
            }
            return sblock;
            break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }
    }

    //! \brief Free given no longer used temporary swapable_block.
    //! \param sblock Temporary swapable_block to free.
    void free_temporary_swapable_block(SwapableBlockType * sblock)
    {
        switch (mode) {
        case online:
            if (sblock->is_acquired())
            {
                //todo error or catch error on deinitialize
            }

            deinitialize(sblock);
            free_temporary_swapable_blocks.push(temporary_swapable_blocks_index(sblock));
            break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }
    }

    //! \brief Turn on simulation mode and reset prediction sequence.
    void start_recording_prediction_sequence()
    {
        switch (mode) {
        case online:
            //todo
            break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }
    }

    //! \brief Turn off simulation mode. Returns a reference to the recorded prediction sequence.
    const prediction_sequence_type & stop_recording_prediction_sequence()
    {
        switch (mode) {
        case online:
            //todo
           break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }//todo
    }

    //! \brief Turn on offline_lfd mode. Use the last used (or recorded) prediction sequence.
    void start_using_prediction_sequence()
    {
        switch (mode) {
        case online:
            //todo
           break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }//todo
    }

    //! \brief Turn on offline_lfd mode. Use the given prediction sequence.
    //! \param new_ps prediction sequence to copy and use.
    void start_using_prediction_sequence(const prediction_sequence_type & new_ps)
    {
        switch (mode) {
        case online:
            //todo
           break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }//todo
    }

    //! \brief Turn off offline_lfd mode.
    void stop_using_prediction_sequence()
    {
        switch (mode) {
        case online:
            //todo
           break;
        case simulation:
            //todo
            break;
        case offline_lfd:
            //todo
            break;
        }//todo
    }

    //! \brief Returns if simulation mode is on, i.e. if a prediction sequence is beeing recorded.
    bool is_in_simulation_mode() const
    {
        return mode == simulation;
    }
};

__STXXL_END_NAMESPACE

#endif /* STXXL_BLOCK_SCHEDULER_HEADER */
