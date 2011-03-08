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

//#define RW_VERBOSE

#include <stack>
#include <limits>

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/common/addressable_queues.h>

__STXXL_BEGIN_NAMESPACE

//! \brief Holds information to allocate, swap and reload a block of data. Suggested to use with block_scheduler.
//!
//! A swappable_block can be uninitialized, i.e. it holds no data.
//! When access is required, is has to be acquired and released afterwards, so it can be swapped in and out as required.
//! If the stored data is no longer needed, it can get uninitialized, freeing in- and external memory.
//! \tparam ValueType type of contained objects (POD with no references to internal memory).
//! \tparam BlockSize Number of objects in one block.
//!         BlockSize*sizeof(ValueType) must be divisible by 4096.
template <typename ValueType, unsigned BlockSize>
class swappable_block
{
protected:
    static const unsigned_type raw_block_size = BlockSize * sizeof(ValueType);
public:
    typedef typed_block<raw_block_size, ValueType> internal_block_type;
    typedef typename internal_block_type::bid_type external_block_type;

protected:
    mutable external_block_type external_data;      //!external_data.valid if no associated space on disk
    mutable internal_block_type * internal_data;    //NULL if there is no internal memory reserved
    mutable bool dirty;
    mutable int_type reference_count;

    static block_manager * bm;

    void get_external_block() const
    { bm->new_block(striping(), external_data); }

    void free_external_block() const
    {
        bm->delete_block(external_data);
        external_data = external_block_type(); // make invalid
    }

public:
    //! \brief Create in uninitialized state.
    swappable_block()
        : external_data() /*!valid*/, internal_data(0), dirty(false), reference_count(0)
    { bm = block_manager::get_instance(); }

    ~swappable_block() {}

    //! \brief If it has an internal_block. The internal_block implicitly holds valid data.
    bool is_internal() const
    { return internal_data; }

    //! \brief If the external_block does not hold valid data.
    bool is_dirty() const
    { return dirty; }

    //! \brief If it has an external_block.
    bool has_external_block() const
    { return external_data.valid(); }

    //! \brief If it has an external_block that holds valid data.
    bool is_external() const
    { return has_external_block() && ! is_dirty(); }

    //! \brief If it is acquired.
    bool is_acquired() const
    { return reference_count > 0; }

    //! \brief If it holds an internal_block but does not need it.
    bool is_evictable() const
    { return ! is_acquired() && is_internal(); }

    //! \brief If it has some valid data (in- or external).
    bool is_initialized() const
    { return is_internal() || is_external(); }

    //! \brief Invalidate external data if true.
    //! \return is_dirty()
    bool make_dirty_if(const bool make_dirty)
    {
        assert(is_acquired());
        return dirty = make_dirty || dirty;
    }

    //! \brief Acquire the block, i.e. add a reference. Has to be internal.
    //! \return A reference to the data-block.
    const internal_block_type & acquire() const
    {
        assert(is_internal());
        ++ reference_count;
        return * internal_data;
    }

    //! \brief Acquire the block, i.e. add a reference. Has to be internal.
    //! \return A reference to the data-block.
    internal_block_type & acquire()
    {
        assert(is_internal());
        ++ reference_count;
        return * internal_data;
    }

    //! \brief Release the block, i.e. subduct a reference. Has to be acquired.
    void release() const
    {
        assert(is_acquired());
        -- reference_count;
    }

    //! \brief Get a reference to the data-block. Has to be acquired.
    const internal_block_type & get_internal_block() const
    {
        assert(is_acquired());
        return * internal_data;
    }

    //! \brief Get a reference to the data-block. Has to be acquired.
    internal_block_type & get_internal_block()
    {
        assert(is_acquired());
        return * internal_data;
    }

    //! \brief Fill block with default data, is supposed to be overwritten by subclass. Has to be internal.
    void fill_default() {}

    //! \brief Fill block with specific value. Has to be internal.
    void fill(const ValueType & value)
    {
        // get_internal_block checks acquired
        internal_block_type & data = get_internal_block();
        for (typename internal_block_type::iterator it = data.begin(); it != data.end(); ++it)
            *it = value;
    }

    //! \brief Read asyncronusly from external_block to internal_block. Has to be internal and have an external_block.
    //! \return A request pointer to the I/O.
    request_ptr read_async(completion_handler on_cmpl = default_completion_handler()) const
    {
        assert(is_internal());
        assert(has_external_block());
        #ifdef RW_VERBOSE
        STXXL_MSG("reading block");
        #endif
        dirty = false;
        return internal_data->read(external_data, on_cmpl);
    }

    //! \brief Read syncronusly from external_block to internal_block. Has to be internal and have an external_block.
    void read_sync() const
    { read_async()->wait(); }

    //! \brief Write asyncronusly from internal_block to external_block if neccesary.
    //! \return A request pointer to the I/O, an invalid request pointer if not neccesary.
    request_ptr clean_async(completion_handler on_cmpl = default_completion_handler()) const
    {
        if (! is_dirty())
            return request_ptr();
        if (! has_external_block())
            get_external_block();
        #ifdef RW_VERBOSE
        STXXL_MSG("writing block");
        #endif
        dirty = false;
        return internal_data->write(external_data, on_cmpl);
    }

    //! \brief Write syncronusly from internal_block to external_block if neccesary.
    void clean_sync() const
    {
        request_ptr rp = clean_async();
        if (rp.valid())
            rp->wait();
    }

    //! \brief Attach an internal_block, making the block internal. Has to be not internal.
    void attach_internal_block(internal_block_type * iblock) const
    {
        assert(! is_internal());
        internal_data = iblock;
    }

    //! \brief Detach the internal_block. Writes to external_block if neccesary. Has to be evictable.
    //! \return A pointer to the internal_block.
    internal_block_type * detach_internal_block() const
    {
        assert(is_evictable());
        clean_sync();
        internal_block_type * iblock = internal_data;
        internal_data = 0;
        return iblock;
    }

    //! \brief Bring the block in uninitialized state, freeing external and internal memory.
    //! Returns a pointer to the internal_block, NULL if it had none.
    //! \return A pointer to the freed internal_block, NULL if it had none.
    internal_block_type * deinitialize()
    {
        assert(! is_acquired());
        dirty = false;
        // free external_block (so that it becomes invalid and the disk-space can be used again)
        if (has_external_block())
            free_external_block();
        // free internal_block
        internal_block_type * iblock = internal_data;
        internal_data = 0;
        return iblock;
    }

    //! \brief Set the external_block that holds the swappable_block's data. The block gets initialized with it.
    //! \param eblock The external_block holding initial data.
    void initialize(external_block_type eblock)
    {
        assert(! is_initialized());
        external_data = eblock;
    }

    //! \brief Extract the swappable_blocks data in an external_block. The block gets uninitialized.
    //! \return The external_block that holds the swappable_block's data.
    external_block_type extract_external_block()
    {
        assert(! is_internal());
        external_block_type eblock = external_data;
        external_data = external_block_type();
        return eblock;
    }
};

template <typename ValueType, unsigned BlockSize>
block_manager * swappable_block<ValueType, BlockSize>::bm;

template <class SwappableBlockType> class block_scheduler_algorithm;

//! \brief Swaps swappable_blocks and provides swappable_blocks for temporary storage.
//!
//! Simple mode only tries to save I/Os through caching.
//! Features a simulation mode to record access patterns in a prediction sequence.
//!   The prediction sequence can then be used for prefetching during a run in offline mode.
//!   This will only work for algorithms with deterministic, data oblivious access patterns.
//!   In simulation mode no I/O is performed; the data provided is accessible but undefined.
//! Execute mode does caching, prefetching and possibly other optimizations.
//! \tparam SwappableBlockType Type of swappable_blocks to manage. Can be some specialized subclass.
template <class SwappableBlockType>
class block_scheduler : private noncopyable
{
protected:
    // tuning-parameter: To acquire blocks, internal memory has to be allocated.
    // This constant limits the number of internal_blocks allocated at once.
    static const int_type max_internal_blocks_alloc_at_once;

    typedef int_type time_type;

public:
    typedef typename SwappableBlockType::internal_block_type internal_block_type;
    typedef typename SwappableBlockType::external_block_type external_block_type;
    typedef typename std::vector<SwappableBlockType>::size_type swappable_block_identifier_type;

    /*/! Mode the block scheduler currently works in
    enum mode
    {
        online,         //serve requests immediately, without any prediction, LRU caching
        simulation,     //record prediction sequence only, do not serve requests, (returned blocks must not be accessed)
        offline_lfd,    //serve requests based on prediction sequence, using longest-forward-distance caching
        offline_lfd_prefetch     //serve requests based on prediction sequence, using longest-forward-distance caching, and prefetching
    };*/

public:
    // -------- prediction_sequence -------
    enum block_scheduler_operation
    {
        op_acquire,
        op_release,
        op_release_dirty,
        op_deinitialize,
        op_initialize,
        op_extract_external_block
    };

    struct prediction_sequence_element
    {
        block_scheduler_operation op;
        swappable_block_identifier_type id;
        time_type time;

        prediction_sequence_element(block_scheduler_operation op,
                swappable_block_identifier_type id, int_type time)
            : op(op), id(id), time(time) {}

    };

    typedef std::vector<prediction_sequence_element> prediction_sequence_type;
    // ---- end prediction_sequence -------

protected:
    template <class SBT> friend class block_scheduler_algorithm;

    const int_type max_internal_blocks;
    int_type remaining_internal_blocks;
    //! \brief Stores pointers to arrays of internal_blocks. Used to deallocate them only.
    std::stack<internal_block_type *> internal_blocks_blocks;
    //! \brief holds free internal_blocks with attributes reset
    std::stack<internal_block_type * > free_internal_blocks;
    //! \brief temporary blocks that will not be needed after algorithm termination
    mutable std::vector<SwappableBlockType> swappable_blocks;
    //! \brief holds indices of free swappable_blocks with attributes reset
    std::stack<swappable_block_identifier_type> free_swappable_blocks;
    block_manager * bm;
    block_scheduler_algorithm<SwappableBlockType> * algo;

    //! \brief Get an internal_block from the freelist or a newly allocated one if available.
    //! \return Pointer to the internal_block. NULL if none available.
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
            int_type num_blocks = std::min(max_internal_blocks_alloc_at_once, remaining_internal_blocks);
            remaining_internal_blocks -= num_blocks;
            internal_block_type * iblocks = new internal_block_type[num_blocks];
            internal_blocks_blocks.push(iblocks);
            for (int_type i = num_blocks - 1; i > 0; --i)
                free_internal_blocks.push(iblocks + i);
            return iblocks;
        }
        else
        {
            // => no internal_block available
            return 0;
        }
    }

    //! \brief Return an internal_block to the freelist.
    void return_free_internal_block(internal_block_type * iblock)
    { free_internal_blocks.push(iblock); }

public:
    //! \brief create a block_scheduler with empty prediction sequence in simple mode.
    //! \param max_internal_memory Amount of internal memory (in bytes) the scheduler is allowed to use for acquiring, prefetching and caching.
    explicit block_scheduler(const int_type max_internal_memory);

    ~block_scheduler();

    //! \brief Acquire the given block.
    //! Has to be in pairs with release. Pairs may be nested and interleaved.
    //! \return Reference to the block's data.
    //! \param sblock Swappable block to acquire.
    internal_block_type & acquire(const swappable_block_identifier_type sbid);

    //! \brief Release the given block.
    //! Has to be in pairs with acquire. Pairs may be nested and interleaved.
    //! \param sblock Swappable block to release.
    //! \param dirty If the data has been changed, invalidating possible data in external storage.
    void release(const swappable_block_identifier_type sbid, const bool dirty);

    //! \brief Drop all data in the given block, freeing in- and external memory.
    void deinitialize(const swappable_block_identifier_type sbid);

    //! \brief Initialize the swappable_block with the given external_block.
    //!
    //! It will use the the external_block for swapping and take care about it's deallocation. Has to be uninitialized.
    //! \param sbid identifier to the swappable_block
    //! \param eblock external_block a.k.a. bid
    void initialize(const swappable_block_identifier_type sbid, external_block_type eblock);

    //! \brief deinitialize the swappable_block and return it's contents in an external_block.
    //!
    //! \param sbid identifier to the swappable_block
    //! \return external_block a.k.a. bid
    external_block_type extract_external_block(const swappable_block_identifier_type sbid);

    //! \brief check if the swappable_block is initialized
    //! \param sbid identifier to the swappable_block
    //! \return if the swappable_block is initialized
    bool is_initialized(const swappable_block_identifier_type sbid) const;

    //! \brief Record a timestep in the prediction sequence to seperate consecutive acquire rsp. release-operations.
    //! Has an effect only in simulation mode.
    void explicit_timestep();

    //! \brief Get a const reference to given block's data. Block has to be already acquired.
    //! \param sblock Swappable block to access.
    internal_block_type & get_internal_block(const swappable_block_identifier_type sbid) const
    { return swappable_blocks[sbid].get_internal_block(); }

    //! \brief Allocate an uninitialized swappable_block.
    //! \return An identifier of the block.
    swappable_block_identifier_type allocate_swappable_block()
    {
        swappable_block_identifier_type sbid;
        if (free_swappable_blocks.empty())
        {
            // create new swappable_block
            sbid = swappable_blocks.size();
            swappable_blocks.resize(sbid + 1);
            algo->swappable_blocks_resize(sbid + 1);
        }
        else
        {
            // take swappable_block from freelist
            sbid = free_swappable_blocks.top();
            free_swappable_blocks.pop();
        }
        return sbid;
    }

    //! \brief Free given no longer used temporary swappable_block.
    //! \param sblock Temporary swappable_block to free.
    void free_swappable_block(const swappable_block_identifier_type sbid)
    {
        deinitialize(sbid);
        free_swappable_blocks.push(sbid);
    }

    //! \brief Returns if simulation mode is on, i.e. if a prediction sequence is being recorded.
    //! \return If simulation mode is on.
    bool is_simulating() const;

    //! \brief Switch the used algorithm, e.g. to simulation etc..
    //! \param new_algo Pointer to the new algorithm object. Has to be instantiated to the block scheduler (or the old algorithm object).
    //! \return Pointer to the old algorithm object.
    block_scheduler_algorithm<SwappableBlockType> * switch_algorithm_to(block_scheduler_algorithm<SwappableBlockType> * new_algo)
    {
        assert(&new_algo->bs == this);
        block_scheduler_algorithm<SwappableBlockType> * old_algo = algo;
        algo = new_algo;
        return old_algo;
    }

    //! \brief get the prediction_sequence
    //! \return reference to the prediction_sequence
    const prediction_sequence_type & get_prediction_sequence() const
    { return algo->get_prediction_sequence(); }

    void flush()
    {
        while (! algo->evictable_blocks_empty())
            return_free_internal_block(swappable_blocks[algo->evictable_blocks_pop()].detach_internal_block());
    }
};

template <class SwappableBlockType>
class block_scheduler_algorithm : private noncopyable
{
protected:
    typedef block_scheduler<SwappableBlockType> block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename block_scheduler_type::external_block_type external_block_type;
    typedef typename block_scheduler_type::swappable_block_identifier_type swappable_block_identifier_type;
    typedef typename block_scheduler_type::prediction_sequence_type prediction_sequence_type;
    typedef typename block_scheduler_type::time_type time_type;

public:
    block_scheduler_type & bs;
protected:
    std::vector<SwappableBlockType> & swappable_blocks;
    prediction_sequence_type prediction_sequence;

    block_scheduler_algorithm * get_algorithm_from_block_scheduler()
    { return bs.algo; }

    //! \brief Get an internal_block from the block_scheduler.
    //! \return Pointer to the internal_block. NULL if none available.
    internal_block_type * get_free_internal_block_from_block_scheduler()
    { return bs.get_free_internal_block(); }

    //! \brief Return an internal_block to the block_scheduler.
    void return_free_internal_block_to_block_scheduler(internal_block_type * iblock)
    { bs.return_free_internal_block(iblock); }

public:
    block_scheduler_algorithm(block_scheduler_type & bs)
        : bs(bs),
          swappable_blocks(bs.swappable_blocks)
    {}

    block_scheduler_algorithm(block_scheduler_algorithm * old)
        : bs(old->bs),
          swappable_blocks(bs.swappable_blocks)
    {}

    virtual ~block_scheduler_algorithm() {};

    virtual bool evictable_blocks_empty() = 0;
    virtual swappable_block_identifier_type evictable_blocks_pop() = 0;
    virtual void swappable_blocks_resize(swappable_block_identifier_type /*size*/) {}

    virtual internal_block_type & acquire(swappable_block_identifier_type sbid) = 0;
    virtual void release(swappable_block_identifier_type sbid, const bool dirty) = 0;
    virtual void deinitialize(swappable_block_identifier_type sbid) = 0;
    virtual void initialize(swappable_block_identifier_type sbid, external_block_type eblock) = 0;
    virtual external_block_type extract_external_block(swappable_block_identifier_type sbid) = 0;

    virtual bool is_initialized(const swappable_block_identifier_type sbid) const
        { return swappable_blocks[sbid].is_initialized(); }

    virtual void explicit_timestep() {}
    virtual bool is_simulating() const
        { return false; }
    virtual const prediction_sequence_type & get_prediction_sequence() const
        { return prediction_sequence; }
};

template <class SwappableBlockType>
class block_scheduler_algorithm_online_lru : public block_scheduler_algorithm<SwappableBlockType>
{
protected:
    typedef block_scheduler<SwappableBlockType> block_scheduler_type;
    typedef block_scheduler_algorithm<SwappableBlockType> block_scheduler_algorithm_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename block_scheduler_type::external_block_type external_block_type;
    typedef typename block_scheduler_type::swappable_block_identifier_type swappable_block_identifier_type;

    using block_scheduler_algorithm_type::bs;
    using block_scheduler_algorithm_type::swappable_blocks;
    using block_scheduler_algorithm_type::get_algorithm_from_block_scheduler;
    using block_scheduler_algorithm_type::get_free_internal_block_from_block_scheduler;
    using block_scheduler_algorithm_type::return_free_internal_block_to_block_scheduler;

    //! \brief Holds swappable blocks, whose internal block can be freed, i.e. that are internal but unacquired.
    addressable_fifo_queue<swappable_block_identifier_type> evictable_blocks;

    internal_block_type * get_free_internal_block()
    {
        // try to get a free internal_block
        if (internal_block_type * iblock = get_free_internal_block_from_block_scheduler())
            return iblock;
        // evict block
        assert(! evictable_blocks.empty()); // fails it there is not enough memory available
        return swappable_blocks[evictable_blocks.pop()].detach_internal_block();
    }

    void return_free_internal_block(internal_block_type * iblock)
    { return_free_internal_block_to_block_scheduler(iblock); }

    void init()
    {
        if (get_algorithm_from_block_scheduler())
            while (! get_algorithm_from_block_scheduler()->evictable_blocks_empty())
                evictable_blocks.insert(get_algorithm_from_block_scheduler()->evictable_blocks_pop());
    }

public:
    block_scheduler_algorithm_online_lru(block_scheduler_type & bs)
        : block_scheduler_algorithm_type(bs)
    { init(); }

    block_scheduler_algorithm_online_lru(block_scheduler_algorithm_type * old)
        : block_scheduler_algorithm_type(old)
    { init(); }

    virtual ~block_scheduler_algorithm_online_lru()
    {
        if (! evictable_blocks.empty())
            STXXL_ERRMSG("Destructing block_scheduler_algorithm_online that still holds evictable blocks. They get deinitialized.");
        while (! evictable_blocks.empty())
        {
            SwappableBlockType & sblock = swappable_blocks[evictable_blocks.pop()];
            if (internal_block_type * iblock = sblock.deinitialize())
                return_free_internal_block(iblock);
        }
    }

    virtual bool evictable_blocks_empty()
    { return evictable_blocks.empty(); }

    virtual swappable_block_identifier_type evictable_blocks_pop()
    { return evictable_blocks.pop(); }

    virtual internal_block_type & acquire(swappable_block_identifier_type sbid)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        /* acquired => internal -> increase reference count
           internal but not acquired -> remove from evictable_blocks, increase reference count
           not intern => uninitialized or external -> get internal_block, increase reference count
           uninitialized -> fill with default value
           external -> read */
        if (sblock.is_internal())
        {
            if (! sblock.is_acquired())
                // not acquired yet -> remove from evictable_blocks
                evictable_blocks.erase(sbid);
            sblock.acquire();
        }
        else if (sblock.is_initialized())
        {
            // => external but not internal
            //get internal_block
            sblock.attach_internal_block(get_free_internal_block());
            //load block synchronously
            sblock.read_sync();
            sblock.acquire();
        }
        else
        {
            // => ! sblock.is_initialized()
            //get internal_block
            sblock.attach_internal_block(get_free_internal_block());
            sblock.acquire();
            //initialize new block
            sblock.fill_default();
        }
        return sblock.get_internal_block();
    }

    virtual void release(swappable_block_identifier_type sbid, const bool dirty)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        sblock.make_dirty_if(dirty);
        sblock.release();
        if (! sblock.is_acquired())
        {
            if (sblock.is_dirty() || sblock.is_external())
                // => evictable, put in pq
                evictable_blocks.insert(sbid);
            else
                // => uninitialized, release internal block and put it in freelist
                return_free_internal_block(sblock.detach_internal_block());
        }
    }

    virtual void deinitialize(swappable_block_identifier_type sbid)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        if (sblock.is_evictable())
            evictable_blocks.erase(sbid);
        if (internal_block_type * iblock = sblock.deinitialize())
            return_free_internal_block(iblock);
    }

    virtual void initialize(swappable_block_identifier_type sbid, external_block_type eblock)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        sblock.initialize(eblock);
    }

    virtual external_block_type extract_external_block(swappable_block_identifier_type sbid)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        if (sblock.is_evictable())
            evictable_blocks.erase(sbid);
        if (sblock.is_internal())
            return_free_internal_block(sblock.detach_internal_block());
        return sblock.extract_external_block();
    }
};

template <class SwappableBlockType>
class block_scheduler_algorithm_simulation : public block_scheduler_algorithm<SwappableBlockType>
{
protected:
    typedef block_scheduler<SwappableBlockType> block_scheduler_type;
    typedef block_scheduler_algorithm<SwappableBlockType> block_scheduler_algorithm_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename block_scheduler_type::external_block_type external_block_type;
    typedef typename block_scheduler_type::swappable_block_identifier_type swappable_block_identifier_type;
    typedef typename block_scheduler_type::prediction_sequence_element prediction_sequence_element_type;
    typedef typename block_scheduler_algorithm_type::time_type time_type;

    using block_scheduler_algorithm_type::bs;
    using block_scheduler_algorithm_type::prediction_sequence;
    using block_scheduler_algorithm_type::swappable_blocks;
    using block_scheduler_algorithm_type::get_algorithm_from_block_scheduler;
    using block_scheduler_algorithm_type::get_free_internal_block_from_block_scheduler;
    using block_scheduler_algorithm_type::return_free_internal_block_to_block_scheduler;

    //! \brief Holds swappable blocks, whose internal block can be freed, i.e. that are internal but unacquired.
    std::stack<swappable_block_identifier_type> evictable_blocks;
    time_type time_count;
    bool last_op_release;
    std::vector<int_type> reference_counts;
    internal_block_type dummy_block;

    void return_free_internal_block(internal_block_type * iblock)
    { return_free_internal_block_to_block_scheduler(iblock); }

    void init()
    {
        if (get_algorithm_from_block_scheduler())
            while (! get_algorithm_from_block_scheduler()->evictable_blocks_empty())
                evictable_blocks.push(get_algorithm_from_block_scheduler()->evictable_blocks_pop());
        for (swappable_block_identifier_type i = 0; i < reference_counts.size(); ++i)
            reference_counts[i] = swappable_blocks[i].is_initialized();
    }

public:
    block_scheduler_algorithm_simulation(block_scheduler_type & bs)
        : block_scheduler_algorithm_type(bs),
          time_count(0),
          last_op_release(false),
          reference_counts(swappable_blocks.size())
    { init(); }

    block_scheduler_algorithm_simulation(block_scheduler_algorithm_type * old)
        : block_scheduler_algorithm_type(old),
          time_count(0),
          last_op_release(false),
          reference_counts(swappable_blocks.size())
    { init(); }

    virtual ~block_scheduler_algorithm_simulation()
    {
        if (! evictable_blocks.empty())
            STXXL_ERRMSG("Destructing block_scheduler_algorithm_record_prediction_sequence that still holds evictable blocks. They get deinitialized.");
        while (! evictable_blocks.empty())
        {
            SwappableBlockType & sblock = swappable_blocks[evictable_blocks.top()];
            if (internal_block_type * iblock = sblock.deinitialize())
                return_free_internal_block(iblock);
            evictable_blocks.pop();
        }
    }

    virtual bool evictable_blocks_empty()
    { return evictable_blocks.empty(); }

    virtual swappable_block_identifier_type evictable_blocks_pop()
    {
        swappable_block_identifier_type sbid = evictable_blocks.top();
        evictable_blocks.pop();
        return sbid;
    }

    virtual internal_block_type & acquire(swappable_block_identifier_type sbid)
    {
        ++reference_counts[sbid];
        last_op_release = false;
        prediction_sequence.push_back(prediction_sequence_element_type
                (block_scheduler_type::op_acquire, sbid, time_count));
        return dummy_block;
    }

    virtual void release(swappable_block_identifier_type sbid, const bool dirty)
    {
        --reference_counts[sbid] += dirty;
        time_count += ! last_op_release;
        last_op_release = true;
        if (dirty)
            prediction_sequence.push_back(prediction_sequence_element_type
                    (block_scheduler_type::op_release_dirty, sbid, time_count));
        else
            prediction_sequence.push_back(prediction_sequence_element_type
                    (block_scheduler_type::op_release, sbid, time_count));
    }

    virtual void deinitialize(swappable_block_identifier_type sbid)
    {
        reference_counts[sbid] = false;
        prediction_sequence.push_back(prediction_sequence_element_type
                (block_scheduler_type::op_deinitialize, sbid, time_count));
    }

    virtual void initialize(swappable_block_identifier_type sbid, external_block_type)
    {
        reference_counts[sbid] = true;
        prediction_sequence.push_back(prediction_sequence_element_type
                (block_scheduler_type::op_initialize, sbid, time_count));
    }

    virtual external_block_type extract_external_block(swappable_block_identifier_type sbid)
    {
        reference_counts[sbid] = false;
        prediction_sequence.push_back(prediction_sequence_element_type
                (block_scheduler_type::op_extract_external_block, sbid, time_count));
        return external_block_type();
    }

    virtual void swappable_blocks_resize(swappable_block_identifier_type size)
    {
        reference_counts.resize(size, 0);
    }

    virtual bool is_initialized(const swappable_block_identifier_type sbid) const
    {
        return reference_counts[sbid] > 0;
    }

    virtual void explicit_timestep()
    { ++time_count; };

    virtual bool is_simulating() const
    { return true; }
};

template <class SwappableBlockType>
class block_scheduler_algorithm_offline_lfd : public block_scheduler_algorithm<SwappableBlockType>
{
protected:
    typedef block_scheduler<SwappableBlockType> block_scheduler_type;
    typedef block_scheduler_algorithm<SwappableBlockType> block_scheduler_algorithm_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename block_scheduler_type::external_block_type external_block_type;
    typedef typename block_scheduler_type::swappable_block_identifier_type swappable_block_identifier_type;
    typedef typename block_scheduler_algorithm_type::time_type time_type;
    typedef typename block_scheduler_type::prediction_sequence_type prediction_sequence_type;

    using block_scheduler_algorithm_type::bs;
    using block_scheduler_algorithm_type::swappable_blocks;
    using block_scheduler_algorithm_type::get_algorithm_from_block_scheduler;
    using block_scheduler_algorithm_type::get_free_internal_block_from_block_scheduler;
    using block_scheduler_algorithm_type::return_free_internal_block_to_block_scheduler;

    class priority
    {
        unsigned_type p;

    public:
        priority(const SwappableBlockType & sblock, const std::pair<bool, time_type> & t)
        {
            // p larger => evict earlier
            if (t.first)
            {
                // most significant: next use
                p = unsigned_type(t.second) << 2;
                // less significant: not dirty
                p |= unsigned_type(! sblock.is_dirty()) << 1;
                // less significant: has external_block
                p |= unsigned_type(sblock.has_external_block()) << 0;
            }
            else
            {
                // most significant: next use
                p = std::numeric_limits<unsigned_type>::max() << 2;
                // less significant: next operation: extract > accessed no more > deinitialize
                p |= unsigned_type(t.second) << 0;
            }
        }

        // less => evict earlier
        bool operator < (const priority & right) const
        { return p > right.p; }
    };

    //! \brief Holds swappable blocks, whose internal block can be freed, i.e. that are internal but unacquired.
    addressable_priority_queue<swappable_block_identifier_type, priority> evictable_blocks;
    /*! \brief stores for the sequence of releases extracted from the prediction_sequence:
            (true, timestamp of the blocks next acquire) if it is acquired next
            (false, 0) if it is deinitialized next
            (false, 1) if it is not accessed any more
            (false, 2) if it is extracted next
            (false, 3) if it is initialized next */
    std::deque< std::pair<bool, time_type> > next_use;

    internal_block_type * get_free_internal_block()
    {
        // try to get a free internal_block
        if (internal_block_type * iblock = get_free_internal_block_from_block_scheduler())
            return iblock;
        // evict block
        assert(! evictable_blocks.empty()); // fails it there is not enough memory available
        return swappable_blocks[evictable_blocks.pop()].detach_internal_block();
    }

    void return_free_internal_block(internal_block_type * iblock)
    { return_free_internal_block_to_block_scheduler(iblock); }

    void init(block_scheduler_algorithm_type * old_algo)
    {
        std::vector< std::pair<bool, time_type> >
                blocks_next_acquire(swappable_blocks.size(), std::make_pair(false, 1));
        if(old_algo)
        {
            // precomputations for priorities: init next_acquires
            const prediction_sequence_type & ps = old_algo->get_prediction_sequence();
            for (typename prediction_sequence_type::const_reverse_iterator it = ps.rbegin(); it != ps.rend(); ++it)
                switch (it->op)
                {
                case (block_scheduler_type::op_acquire):
                    blocks_next_acquire[it->id] = std::make_pair(true, it->time);
                    break;
                case (block_scheduler_type::op_release):
                case (block_scheduler_type::op_release_dirty):
                    next_use.push_front(blocks_next_acquire[it->id]);
                    break;
                case (block_scheduler_type::op_deinitialize):
                    blocks_next_acquire[it->id] = std::make_pair(false, 0);
                    break;
                case (block_scheduler_type::op_initialize):
                    blocks_next_acquire[it->id] = std::make_pair(false, 3);
                    break;
                case (block_scheduler_type::op_extract_external_block):
                    blocks_next_acquire[it->id] = std::make_pair(false, 2);
                    break;
                }
        }
        if (get_algorithm_from_block_scheduler())
        {
            while (! get_algorithm_from_block_scheduler()->evictable_blocks_empty())
            {
                // insert already evictable blocks with the right priority
                const swappable_block_identifier_type sbid = get_algorithm_from_block_scheduler()->evictable_blocks_pop();
                evictable_blocks.insert(sbid, priority(swappable_blocks[sbid], blocks_next_acquire[sbid]));
            }
        }
    }

public:
    block_scheduler_algorithm_offline_lfd(block_scheduler_type & bs)
        : block_scheduler_algorithm_type(bs)
    { init(get_algorithm_from_block_scheduler()); }

    // It is possible to keep an old simulation-algorithm object and reuse it's prediction sequence
    block_scheduler_algorithm_offline_lfd(block_scheduler_algorithm_type * old)
        : block_scheduler_algorithm_type(old)
    { init(old); }

    virtual ~block_scheduler_algorithm_offline_lfd()
    {
        if (! evictable_blocks.empty())
            STXXL_ERRMSG("Destructing block_scheduler_algorithm_offline_lfd that still holds evictable blocks. They get deinitialized.");
        while (! evictable_blocks.empty())
        {
            SwappableBlockType & sblock = swappable_blocks[evictable_blocks.pop()];
            if (internal_block_type * iblock = sblock.deinitialize())
                return_free_internal_block(iblock);
        }
    }

    virtual bool evictable_blocks_empty()
    { return evictable_blocks.empty(); }

    virtual swappable_block_identifier_type evictable_blocks_pop()
    { return evictable_blocks.pop(); }

    virtual internal_block_type & acquire(swappable_block_identifier_type sbid)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        /* acquired => internal -> increase reference count
           internal but not acquired -> remove from evictable_blocks, increase reference count
           not intern => uninitialized or external -> get internal_block, increase reference count
           uninitialized -> fill with default value
           external -> read */
        if (sblock.is_internal())
        {
            if (! sblock.is_acquired())
                // not acquired yet -> remove from evictable_blocks
                evictable_blocks.erase(sbid);
            sblock.acquire();
        }
        else if (sblock.is_initialized())
        {
            // => external but not internal
            //get internal_block
            sblock.attach_internal_block(get_free_internal_block());
            //load block synchronously
            sblock.read_sync();
            sblock.acquire();
        }
        else
        {
            // => ! sblock.is_initialized()
            //get internal_block
            sblock.attach_internal_block(get_free_internal_block());
            sblock.acquire();
            //initialize new block
            sblock.fill_default();
        }
        return sblock.get_internal_block();
    }

    virtual void release(swappable_block_identifier_type sbid, const bool dirty)
    {
        if (next_use.empty())
        {
            STXXL_ERRMSG("block_scheduler_algorithm_offline_lfd got release-request but prediction sequence ended. Switching to block_scheduler_algorithm_online.");
            // switch algorithm
            block_scheduler_algorithm_type * new_algo,
                                           * old_algo;
            new_algo = new block_scheduler_algorithm_online_lru<SwappableBlockType>(bs);
            old_algo = bs.switch_algorithm_to(new_algo);
            // redirect call
            new_algo->release(sbid, dirty);
            // delete self
            delete old_algo;
            return;
        }
        SwappableBlockType & sblock = swappable_blocks[sbid];
        sblock.make_dirty_if(dirty);
        sblock.release();
        if (! sblock.is_acquired())
        {
            if (sblock.is_dirty() || sblock.is_external())
                // => evictable, put in pq
                evictable_blocks.insert(sbid, priority(swappable_blocks[sbid], next_use.front()));
            else
                // => uninitialized, release internal block and put it in freelist
                return_free_internal_block(sblock.detach_internal_block());
        }
        next_use.pop_front();
    }

    virtual void deinitialize(swappable_block_identifier_type sbid)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        if (sblock.is_evictable())
            evictable_blocks.erase(sbid);
        if (internal_block_type * iblock = sblock.deinitialize())
            return_free_internal_block(iblock);
    }

    virtual void initialize(swappable_block_identifier_type sbid, external_block_type eblock)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        sblock.initialize(eblock);
    }

    virtual external_block_type extract_external_block(swappable_block_identifier_type sbid)
    {
        SwappableBlockType & sblock = swappable_blocks[sbid];
        if (sblock.is_evictable())
            evictable_blocks.erase(sbid);
        if (sblock.is_internal())
            return_free_internal_block(sblock.detach_internal_block());
        return sblock.extract_external_block();
    }
};

// +-+-+-+-+-+-+ definitions of wrapping block_scheduler operations +-+-+-+-+-+-+-+-+-+-+-+-+

template <class SwappableBlockType>
block_scheduler<SwappableBlockType>::block_scheduler(const int_type max_internal_memory)
    : max_internal_blocks(div_ceil(max_internal_memory, sizeof(internal_block_type))),
      remaining_internal_blocks(max_internal_blocks),
      bm(block_manager::get_instance()),
      algo(0)
{ algo = new block_scheduler_algorithm_online_lru<SwappableBlockType>(*this); }

template <class SwappableBlockType>
block_scheduler<SwappableBlockType>::~block_scheduler()
{
    delete algo;
    int_type num_freed_internal_blocks = 0;
    if (free_swappable_blocks.size() != swappable_blocks.size())
    {
        // => not all swappable_blocks are free, at least deinitialize them
        STXXL_ERRMSG("not all swappable_blocks are free, those not acquired will be deinitialized");
        for (typename std::vector<SwappableBlockType>::iterator it = swappable_blocks.begin(); // evictable_blocks would suffice
                it != swappable_blocks.end(); ++it)
            if (! it->is_acquired())
                num_freed_internal_blocks += bool(it->deinitialize()); // count internal_blocks that get freed
    }
    if (int_type nlost = (max_internal_blocks - remaining_internal_blocks)
            - (free_internal_blocks.size() + num_freed_internal_blocks))
        STXXL_ERRMSG(nlost << " internal_blocks are lost. They will get deallocated.");
    while (! internal_blocks_blocks.empty())
    {
        delete [] internal_blocks_blocks.top();
        internal_blocks_blocks.pop();
    }
}

template <class SwappableBlockType>
const int_type block_scheduler<SwappableBlockType>::max_internal_blocks_alloc_at_once = 128;

template <class SwappableBlockType>
typename block_scheduler<SwappableBlockType>::internal_block_type & block_scheduler<SwappableBlockType>::acquire(const swappable_block_identifier_type sbid)
{ return algo->acquire(sbid); }

template <class SwappableBlockType>
void block_scheduler<SwappableBlockType>::release(const swappable_block_identifier_type sbid, const bool dirty)
{ algo->release(sbid, dirty); }

template <class SwappableBlockType>
void block_scheduler<SwappableBlockType>::deinitialize(const swappable_block_identifier_type sbid)
{ algo->deinitialize(sbid); }

template <class SwappableBlockType>
void block_scheduler<SwappableBlockType>::initialize(const swappable_block_identifier_type sbid, external_block_type eblock)
{ algo->initialize(sbid, eblock); }

template <class SwappableBlockType>
typename block_scheduler<SwappableBlockType>::external_block_type block_scheduler<SwappableBlockType>::extract_external_block(const swappable_block_identifier_type sbid)
{ return algo->extract_external_block(sbid); }

template <class SwappableBlockType>
bool block_scheduler<SwappableBlockType>::is_initialized(const swappable_block_identifier_type sbid) const
{ return algo->is_initialized(sbid); }

template <class SwappableBlockType>
void block_scheduler<SwappableBlockType>::explicit_timestep()
{ algo->explicit_timestep(); }

template <class SwappableBlockType>
bool block_scheduler<SwappableBlockType>::is_simulating() const
{ return algo->is_simulating(); }

// +-+-+-+-+ end definitions of wrapping block_scheduler operations +-+-+-+-+-+-+-+-+-+-+-+-+

__STXXL_END_NAMESPACE

#endif /* STXXL_BLOCK_SCHEDULER_HEADER */
