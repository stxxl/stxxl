/*!
 * External array stores a sorted sequence of values on the hard disk and
 * allows access to the first block (containing the smallest values).  The
 * class uses buffering and prefetching in order to improve the performance.
 *
 * \tparam ValueType Type of the contained objects (POD with no references to
 * internal memory).
 *
 * \tparam BlockSize External block size. Default =
 * STXXL_DEFAULT_BLOCK_SIZE(ValueType).
 *
 * \tparam AllocStrategy Allocation strategy for the external memory. Default =
 * STXXL_DEFAULT_ALLOC_STRATEGY.
 */
template <
    class ValueType,
    unsigned_type BlockSize = STXXL_DEFAULT_BLOCK_SIZE(ValueType),
    class AllocStrategy = STXXL_DEFAULT_ALLOC_STRATEGY
    >
class external_array : private noncopyable
{

public:
    
    typedef ValueType value_type;
    typedef ppq_iterator<value_type> iterator;

protected:

    typedef external_array<ValueType, BlockSize, AllocStrategy> self_type;
    typedef typed_block<BlockSize, ValueType> block_type;
    typedef read_write_pool<block_type> pool_type;
    typedef std::vector< BID<BlockSize> > bid_vector;
    typedef std::vector< block_type* > block_vector;
    typedef std::vector< request_ptr > request_vector;
    typedef std::vector< ValueType > maxima_vector;
    typedef typename iterator::block_pointers_type block_pointers_type;

public:

    //! The number of elements fitting into one block
    static const size_t block_size = (size_t) BlockSize / sizeof(ValueType);
    
protected:
    
    //! The total capacity of the external array. Cannot be changed after construction.
    size_t m_capacity;

    //! Number of blocks
    size_t m_num_blocks;

    //! Number of blocks to prefetch from EM
    size_t m_num_prefetch_blocks;

    //! Size of the write buffer in number of blocks
    size_t m_num_write_buffer_blocks;

    //! Prefetch and write buffer pool
    pool_type* m_pool;

    //! The IDs of each block in external memory
    bid_vector m_bids;

    //! The block with the currently smallest elements
    block_vector m_blocks;
    
    //! Begin and end pointers for each block
    block_pointers_type m_block_pointers;

    //! The read request pointers are used to wait until the block has been completely fetched.
    request_vector m_requests;

    //! stores the maximum value of each block
    maxima_vector m_maxima;

    //! Is array in write phase? True = write phase, false = read phase.
    bool m_write_phase;

    //! The total number of elements minus the number of extracted values
    size_t m_size;

    //! The read position in the array.
    size_t m_index;
    
    //! The index behind the last element that is located in RAM (or is at least requested to be so)
    size_t m_end_index;

    //! The index of the next block to be prefetched.
    size_t m_hint_index;

public:

    /*!
     * Constructs an external array
     *
     * \param size The total number of elements. Cannot be changed after
     * construction.
     *
     * \param num_prefetch_blocks Number of blocks to prefetch from hard disk
     *
     * \param num_write_buffer_blocks Size of the write buffer in number of
     * blocks
     */
    external_array(size_t size, size_t num_prefetch_blocks,
                   size_t num_write_buffer_blocks)
        :   // constants
            m_capacity(size),
            m_num_blocks((size_t)div_ceil(m_capacity, block_size)),
            m_num_prefetch_blocks(std::min(num_prefetch_blocks, m_num_blocks)),
            m_num_write_buffer_blocks(std::min(num_write_buffer_blocks, m_num_blocks)),
            m_pool(new pool_type(0, m_num_write_buffer_blocks)),

            // vectors
            m_bids(m_num_blocks),
            m_blocks(m_num_blocks, NULL),
            m_block_pointers(m_num_blocks),
            m_requests(m_num_blocks, NULL),
            m_maxima(m_num_blocks),
            
            // state
            m_write_phase(true),
            
            // indices
            m_size(0),
            m_index(0),
            m_end_index(1), // buffer size must be at least 1
            m_hint_index(1)
    {
        assert(m_capacity > 0);
        assert(m_num_write_buffer_blocks > 0); // needed for steal()
        // allocate blocks in EM.
        block_manager* bm = block_manager::get_instance();
        // first BID is not used, because first block is never written to EM.
        bm->new_blocks(AllocStrategy(), m_bids.begin()+1, m_bids.end());
        m_blocks[0] = new block_type;
        update_block_pointers(0);
    }

    //! Default constructor. Don't use this directy. Needed for regrowing in
    //! surrounding vector.
    external_array()
        :   // constants
            m_capacity(0),
            m_num_blocks(0),
            m_num_prefetch_blocks(0),
            m_num_write_buffer_blocks(0),
            m_pool(NULL),

            // vectors
            m_bids(0),
            m_blocks(0),
            m_block_pointers(0),
            m_requests(0),
            m_maxima(0),
            
            // state
            m_write_phase(false),
            
            // indices
            m_size(0),
            m_index(0),
            m_end_index(0),
            m_hint_index(0)
    { }

    //! Swap internal_array with another one.
    void swap(external_array& o)
    {
        // constants
        std::swap(m_capacity, o.m_capacity);
        std::swap(m_num_blocks, o.m_num_blocks);
        std::swap(m_num_prefetch_blocks, o.m_num_prefetch_blocks);
        std::swap(m_num_write_buffer_blocks, o.m_num_write_buffer_blocks);
        std::swap(m_pool, o.m_pool);
        
        // vectors
        std::swap(m_bids, o.m_bids);
        std::swap(m_requests, o.m_requests);
        std::swap(m_blocks, o.m_blocks);
        std::swap(m_block_pointers, o.m_block_pointers);
        std::swap(m_maxima, o.m_maxima);
        
        // state
        std::swap(m_write_phase, o.m_write_phase);
        
        // indices
        std::swap(m_size, o.m_size);
        std::swap(m_index, o.m_index);
        std::swap(m_end_index, o.m_end_index);
        std::swap(m_hint_index, o.m_hint_index);
    }

    //! Destructor
    ~external_array()
    {
        if (m_size>0) {
        
            // not all data has been read!
            
            assert(m_end_index>0); // true because m_size>0
            const size_t block_index = m_index / block_size;
            const size_t end_block_index = (m_end_index-1) / block_size;
            
            block_manager* bm = block_manager::get_instance();
            typename bid_vector::iterator i_begin = m_bids.begin()+std::max(block_index,(size_t)1);
            bm->delete_blocks(i_begin,m_bids.end());
            
            for (size_t i=block_index; i<end_block_index; ++i) {
                delete m_blocks[i];
            }
            
        }
        delete m_pool;
    }

    //! Returns the current size
    size_t size() const
    {
        return m_size;
    }

    //! Returns if the array is empty
    bool empty() const
    {
        return (m_size == 0);
    }

    //! Returns if the array is empty
    size_t buffer_size() const
    {
        return (m_end_index-m_index);
    }
    
    //! Returns a random-access iterator to the begin of the data
    //! in internal memory.
    iterator begin() {
        assert(m_index == m_capacity || block_valid(m_index/block_size));
        // not const, unfortunately.
        return iterator(m_block_pointers, block_size, m_index);
    }
    
    //! Returns a random-access iterator 1 behind the end of the data
    //! in internal memory.
    iterator end() {
        assert(m_end_index == m_index || block_valid((m_end_index-1)/block_size));
        // not const, unfortunately.
        return iterator(m_block_pointers, block_size, m_end_index);
    }

    //! Returns the smallest element in the array
    ValueType & get_min()
    {
        return *begin();
    }
    
    //! Returns the largest element in internal memory (or at least
    //! requested to be in internal memory)
    ValueType& get_current_max() {
        assert(m_end_index>0);
        const size_t last_block_index = (m_end_index-1) / block_size;
        return m_maxima[last_block_index];
    }
    
    //! Returns if there is data in EM, that's not randomly accessible.
    bool has_em_data() const {
        assert(m_end_index>0);
        const size_t last_block_index = (m_end_index-1) / block_size;
        return (last_block_index+1 < m_num_blocks);
    }
    
    //! Returns if the data requested to be in internal memory is
    //! completely fetched. True if wait() has been called before.
    bool valid() const {
        bool result = true;
        const size_t block_index = m_index / block_size;
        const size_t end_block_index = (m_end_index-1) / block_size;
        for (size_t i=block_index; i<=end_block_index; ++i) {
            result = result && block_valid(i);
        }
        return result;
    } 

    //! Random access operator for data in internal memory
    //! You should call wait() once after fetching data from EM.
    ValueType & operator [] (size_t i) const
    {
        if (i>=m_capacity) {
            // STXXL_MSG("warning: accessing index "<<i);
            // This is a hack for __gnu_parallel::multiway_merge compatibility.
            // The element one past the last one is accessed by the multiway_merge algorithm.
            return m_blocks[0]->elem[0];
        }
        const size_t block_index = i / block_size;
        const size_t local_index = i % block_size;
        assert(i<m_capacity);
        assert(block_valid(block_index));
        return m_blocks[block_index]->elem[local_index];
    }

    //! Pushes the value at the end of the array
    void push_back(const ValueType& value)
    {
        assert(m_write_phase);
        assert(m_size < m_capacity);
        
        const size_t block_index = m_size / block_size;
        const size_t local_index = m_size % block_size;
        
        m_blocks[block_index]->elem[local_index] = value;
        ++m_size;
        
        // block finished? -> write out.
        if (UNLIKELY(m_size % block_size == 0 || m_size == m_capacity)) {
            m_maxima[block_index] = m_blocks[block_index]->elem[block_size-1];
            if (block_index>0) {
                // first block is never written to EM!
                assert(m_bids[block_index].valid());
                m_pool->write(m_blocks[block_index], m_bids[block_index]);
            }
            if (block_index+1<m_num_blocks) {
                // Steal block for further filling.
                m_blocks[block_index+1] = m_pool->steal();
                update_block_pointers(block_index+1);
            }
        }
        
        // compatibility to the block write interface
        m_index = m_size;
        m_end_index = m_index+1;

    }
    
    //! Request a write buffer of at least the given size.
    //! It can be accessed using begin() and end().
    void request_write_buffer(size_t size) {
        
        assert(m_write_phase);
        assert(size>0);
        assert(m_size+size <= m_capacity);
        assert(m_end_index>m_index);
        
        const size_t block_index = m_index / block_size;
        const size_t last_block_index = (m_end_index-1) / block_size;
        
        m_end_index = m_size+size;
        
        const size_t last_block_index_after = (m_end_index-1) / block_size;
        
        const size_t num_buffer_blocks = std::max(last_block_index_after,(size_t)1)
            - std::max(block_index,(size_t)1) + 1;
        
        if ( num_buffer_blocks > m_pool->size_write() ) {
            // needed for m_pool->steal()
            m_pool->resize_write(num_buffer_blocks);
        }
          
        for (size_t i = last_block_index+1; i <= last_block_index_after; ++i) {
            m_blocks[i] = m_pool->steal();
            update_block_pointers(i);
        }
        
    }
    
    //! Writes the data from the write buffer to EM, if necessary.
    void flush_write_buffer() {
        
        const size_t size = m_end_index - m_index;
        const size_t block_index = m_index / block_size;
        size_t last_block_index = (m_end_index-1) / block_size;
        
        if ( m_size+size == m_capacity ) {
            // we have to flush the last block, too.
            ++last_block_index;
        }
        
        for (size_t i = std::max(block_index,(size_t)1); i<last_block_index; ++i) {
            if (i<m_num_blocks-1) {
                m_maxima[i] = m_blocks[i]->elem[block_size-1];
            } else {
                // last block
                m_maxima[i] = m_blocks[i]->elem[last_block_size()-1];
            }
            assert(m_bids[i].valid());
            m_pool->write(m_blocks[i], m_bids[i]);
        }
        
        if ( m_size+size < m_capacity
                && m_end_index % block_size == 0
                && last_block_index+1 < m_num_blocks ) {
            // block was full -> request new one
            m_blocks[last_block_index+1] = m_pool->steal();
            update_block_pointers(last_block_index+1);
        }
        
        m_size += size;
        m_index = m_size;
        m_end_index = m_index+1; // buffer size must be at least 1
        
    }

    //! Finish write phase. Afterwards the values can be extracted from bottom
    //! up (ascending order).
    void finish_write_phase()
    {
        assert(m_capacity == m_size);
        m_pool->resize_write(0);
        m_pool->resize_prefetch(m_num_prefetch_blocks);
        m_write_phase = false;
        
        // set m_maxima[0]
        if (m_capacity<=block_size) {
            m_maxima[0] = m_blocks[0]->elem[m_capacity-1];
        } else {
            m_maxima[0] = m_blocks[0]->elem[block_size-1];
        }
        
        const size_t local_block_size = block_size; // std::min cannot access static block_size
        m_end_index = std::min(local_block_size,m_capacity);
        m_index = 0;

        for (size_t i = 0; i < m_num_prefetch_blocks; ++i) {
            hint();
        }

        assert(m_blocks[0]);
        
    }
    
    //! Waits for requested data to be completely fetched into
    //! internal memory. Call this if you want to read data after
    //! remove() or request_further_block() has been called.
    void wait() {
    
        assert(!m_write_phase);
        assert(m_end_index>0);
        const size_t block_index = m_index / block_size;
        const size_t end_block_index = (m_end_index-1) / block_size;
        
        // ignore first block (never in EM)
        for (size_t i=std::max((size_t)1,block_index); i<=end_block_index; ++i) {
            assert(m_requests[i]);
            m_requests[i]->wait();
            assert(m_requests[i]->poll());
            assert(m_blocks[i]);
        }
        
    }

    //! Removes the first n elements from the array.  Loads the
    //! next block if the current one has run empty. Make shure there
    //! are at least n elements in RAM.
    void remove(size_t n)
    {
        assert(m_index+n<=m_capacity);
        assert(m_index+n<=m_end_index);
        assert(m_size>=n);
        assert(n>0);

        const size_t block_index = m_index / block_size;
        
        const size_t index_after = m_index+n;
        const size_t block_index_after = index_after / block_size;
        const size_t local_index_after = index_after % block_size;
        
        assert(block_index_after <= m_num_blocks);
        
        block_manager* bm = block_manager::get_instance();
        typename bid_vector::iterator i_begin = m_bids.begin()+std::max(block_index,(size_t)1);
        typename bid_vector::iterator i_end = m_bids.begin()+std::max(block_index_after,(size_t)1);
        if (i_end>i_begin) {
            bm->delete_blocks(i_begin,i_end);
        }
        
        for (size_t i=block_index; i<block_index_after; ++i) {
            assert(block_valid(i));
            delete m_blocks[i];
        }

        m_index = index_after;
        m_size -= n;

        if (block_index_after >= m_num_blocks) {
            assert(block_index_after == m_num_blocks);
            assert(m_size==0);
        } else if (local_index_after==0) {
            assert(block_index_after==(m_end_index-1)/block_size+1);
            request_further_block();
        } else {
            assert(block_valid(block_index_after));
        }
        
    }

    //! Requests one more block to be fetched into RAM
    void request_further_block() {
    
        assert(has_em_data());
        assert(m_end_index>0);
        
        const size_t block_index = (m_end_index-1)/block_size + 1;
        
        assert(block_index<m_num_blocks);
        assert(block_index>0); // first block is already in RAM
        assert(m_bids[block_index].valid());
        
        m_blocks[block_index] = new block_type;
        m_requests[block_index] = m_pool->read(m_blocks[block_index], m_bids[block_index]);
        update_block_pointers(block_index);
        
        hint();
        m_end_index = std::min(m_capacity,(block_index+1)*block_size);
        
    }
    
protected:

    //! Returns if the block with the given index is completely fetched.
    bool block_valid(size_t block_index) const {
        if (!m_write_phase) {
            return ( (block_index==0) || (m_requests[block_index] && m_requests[block_index]->poll()));
        } else {
            return (bool) m_blocks[block_index];
        }
    }

    //! Gives the prefetcher a hint for the next block to prefetch.
    void hint()
    {
        if (m_hint_index < m_num_blocks) {
            m_pool->hint(m_bids[m_hint_index]);
        }
        ++m_hint_index;
    }
    
    //! Updates the m_block_pointers vector.
    //! Should be called after any steal() or read() operation.
    //! This is necessary for the iterators to work properly.
    inline void update_block_pointers(size_t block_index) {
        m_block_pointers[block_index].first = m_blocks[block_index]->begin();
        m_block_pointers[block_index].second = m_blocks[block_index]->end();
        assert(m_block_pointers[block_index].first!=NULL);
        assert(m_block_pointers[block_index].second!=NULL);
    }
    
    inline size_t last_block_size() {
        size_t mod = m_capacity%block_size;
        return (mod>0) ? mod : block_size;
    }
    
};
