/***************************************************************************
 *  include/stxxl/bits/containers/matrix.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010, 2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define CHECK 1

#ifndef STXXL_MATRIX_HEADER
#define STXXL_MATRIX_HEADER

#ifndef STXXL_BLAS
#define STXXL_BLAS 0
#endif

#include <queue>
#include <stack>
#include <set>

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/containers/matrix_layouts.h>


__STXXL_BEGIN_NAMESPACE


// +-+-+-+-+-+ swapable_block stuff +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// The priority must not change for handles in the queue! Otherwise they can not be found by remove.
template <typename HandleType , typename PriorityType /* = HandleType::priority_type */> //todo default PriorityType
class fixed_priority_adressable_p_queue
{
    typedef PriorityType (* priority_function_type)(const HandleType);
    typedef std::pair<PriorityType, HandleType> element_type;

    const priority_function_type pf;
    std::set<element_type> vals;

public:
    fixed_priority_adressable_p_queue(priority_function_type priority_function)
        : pf(priority_function) { }

    bool empty() const
    { return vals.empty(); }

    void insert(const HandleType h)
    {
        const element_type e(pf(h), h);
        vals.insert(e);
    }

    void remove(const HandleType h)
    {
        const element_type e(pf(h), h);
        vals.erease(e);
    }

    HandleType pop()
    {
        const HandleType h = vals.begin()->second;
        vals.erease(vals.begin());
        return h;
    }
};


//! \brief Internal block container with access-metadata. Not intended for direct use.
//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam BlockSize number of contained objects
template <typename ValueType, unsigned BlockSize>
struct internal_block
{
    static const unsigned_type raw_block_size = BlockSize * sizeof(ValueType);
    typedef typed_block<raw_block_size, ValueType> block_data_holder_type;

    block_data_holder_type data;
    bool dirty;
    int_type reference_count;

    internal_block() : data(), dirty(false), reference_count(0) {}
};

//! \brief Holds infomation to allocate, swap and reload a block of data. Use with block_scheduler.
//! A swapable_block can be uninitialised, i.e. it holds no data.
//! When access is required, is has to be aquired and released afterwards, so it can be swaped in and out as required.
//! If the stored data is no longer needed, it can get uninitialized, freeing in- and external memory.
//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam BlockSize number of contained objects
template <typename ValueType, unsigned BlockSize>
class swapable_block
{
public:
    typedef internal_block<ValueType, BlockSize> internal_block_type;
    typedef typename internal_block_type::block_data_holder_type block_data_holder_type;
protected:
    typedef typename block_data_holder_type::bid_type external_block_type;

    template <class SwapableBlockType, class DiskAssignFunctor>
    friend class block_scheduler;

    external_block_type external_data;
    internal_block_type * internal_data;

    //! \brief create in uninitialized state
    swapable_block()
        : external_data(), internal_data(0) {}

    //! \brief create in initialised, swaped out state
    //! \param bid external_block with data, will be used as swap-space of this swapable_block
    swapable_block(const external_block_type & bid)
        : external_data(bid), internal_data(0) {}

    ~swapable_block()
    {
        if (internal_data)
            STXXL_ERRMSG("Destructing swapable_block that still holds internal_block.");
    }

    bool is_intern() const
    { return internal_data; }

    bool is_aquired() const
    { return internal_data && internal_data->reference_count > 0; }

    int_type & reference_count()
    { return internal_data->reference_count; }

public:
    bool is_initialized() const
    { return internal_data || external_data.valid(); }

    block_data_holder_type & get_reference()
    {
#if CHECK
        if (is_aquired())
#endif
            return internal_data->data;
#if CHECK
        //todo else throw ;
#endif
    }

    void fill_default() {}

    void fill(ValueType value)
    {
        // check aquired
        block_data_holder_type & data = get_reference();
        // fill
        for (int_type i = 0; i < BlockSize; ++i)
            data[i] = value;
    }
};

enum block_scheduler_operation
{
    aquire,
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

template <class SwapableBlockType>
class prediction_sequence_element
{
    typedef swapable_block_identifier<SwapableBlockType> swapable_block_identifier_type;
    typedef typename swapable_block_identifier_type::temporary_swapable_blocks_index_type temporary_swapable_blocks_index_type;

    template <class SwapableBlockType, class DiskAssignFunctor>
    friend class block_scheduler;

protected:
    block_scheduler_operation op;
    swapable_block_identifier_type id;

    prediction_sequence_element(block_scheduler_operation operation, temporary_swapable_blocks_index_type  temporary_swapable_block)
        : op(operation), id(0,temporary_swapable_block) { }

public:
    prediction_sequence_element(block_scheduler_operation operation, SwapableBlockType *  swapable_block)
        : op(operation), id(swapable_block,0) { }
};

enum block_scheduler_mode
{
    simple,  //todo? better name
    simulation,
    execute
};

//! \brief Swaps swapable_blocks and provides swapable_blocks for temporary storage.
//! Simple mode only tries to save I/Os through caching.
//! Features a simulation mode to record access patterns in a prediction sequence.
//!   The prediction sequence can then be used for prefetching during a run in execute mode.
//!   This will only work for algorithms with deterministic, data oblivious access patterns.
//!   In simulation mode no I/O is perfomed; the data provided is accessible but undefined.
//! Execute mode does caching, prefetching and possibly other optimisations.
//! \tparam SwapableBlockType Type of swapable_blocks to manage. Can be some specialised subclass.
template <class SwapableBlockType, class DiskAssignFunctor>
class block_scheduler
{
    typedef typename SwapableBlockType::internal_block_type internal_block_type;
    typedef typename SwapableBlockType::block_data_holder_type block_data_holder_type;
    typedef unsigned_type displaceable_swapable_block_priority_type;
    typedef typename prediction_sequence_element<SwapableBlockType>::temporary_swapable_blocks_index_type temporary_swapable_blocks_index_type;
public:
    typedef std::vector< prediction_sequence_element<SwapableBlockType> > prediction_sequence_type;

protected:
    std::stack<internal_block_type * > free_internal_blocks;
    std::vector<SwapableBlockType> temporary_swapable_blocks;
    std::stack<temporary_swapable_blocks_index_type> free_temporary_swapable_blocks;
    prediction_sequence_type prediction_sequence;
    // holds swapable blocks, whose internal block can be freed, i.e. that are internal but unaquired
    fixed_priority_adressable_p_queue<SwapableBlockType *, displaceable_swapable_block_priority_type> displaceable_swapable_blocks; // todo: better name
    block_scheduler_mode mode;
    internal_block_type * dummy_internal_block;

public:
    //! \brief create a block_scheduler with empty prediction sequence in simple mode.
    //! \param max_internal_memory Amount of internal memory (in sizeof(char)) the scheduler is allowed to use for aquireing, prefetching and caching.
    block_scheduler(int_type max_internal_memory)
        : free_internal_blocks(div_ceil(max_internal_memory, sizeof(block_data_holder_type))),
          displaceable_swapable_blocks(get_block_priority),
          mode(simple)
    {
        //todo
    }

protected:
    internal_block_type * get_free_internal_block()
    {
        if ( ! free_internal_blocks.empty())
        {
            internal_block_type * free_b = free_internal_blocks.top();
            free_internal_blocks.pop();
            return free_b;
        }
        else
        {
            //todo
            //free block from pq intern_swapable_blocks
            //reset block
        }
    }

    displaceable_swapable_block_priority_type get_block_priority(const SwapableBlockType * block) const
    {
        //todo
        return 0;
    }

public:
    //! \brief Aquire the given block.
    //! \return Reference to the blocks's data.
    //! Has to be in pairs with release. Pairs may be nested.
    //! \param sblock Swapable block to aquire.
    block_data_holder_type & aquire(SwapableBlockType * sblock)
    {
        switch (mode) {
        case simple:
            // aquired => intern -> increase reference count
            // intern but not aquired -> remove from displaceable_swapable_blocks, increase reference count
            // not intern => uninitialized or extern -> get internal_block, increase reference count
            // uninitialized -> fill with default value
            // extern -> read
            if (sblock->is_intern)
            {
                // not aquired yet -> remove from pq
                if (! sblock->is_aquired())
                    displaceable_swapable_blocks.remove(sblock);
            }
            else if (sblock->is_initialized())
            {
                //get internal_block
                sblock->internal_data = get_free_internal_block();
                //load block syncronusly
                sblock->internal_data->data.read(sblock->external_data) ->wait();
            }
            else
            {
                //get internal_block
                sblock->internal_data = get_free_internal_block();
                //initialize new block
                sblock->fill_default();
            }
            //increase reference count
            ++ sblock->reference_count();
            break;
        case simulation:
            //todo
            break;
        case execute:
            //todo
            break;
        }
    }

    //! \brief Release the given block.
    //! Has to be in pairs with aquire. Pairs may be nested.
    //! \param sblock Swapable block to release.
    //! \param tainted If the data hab been changed, invalidating possible data in external storage.
    void release(SwapableBlockType * sblock, bool tainted)
    {
        switch (mode) {
        case simple:
            //todo
            break;
        case simulation:
            //todo
            break;
        case execute:
            //todo
            break;
        }
    }

    //! \brief Loose all data in the given block, freeing in- and external memory.
    void deinitialize(SwapableBlockType * sblock)
    {
        //todo
    }

    //! \brief Record a timestep in the prediction sequence to seperate consecutive aquire rsp. release-operations.
    //! Has an effect only in simulation mode.
    void explicit_timestep()
    {
        //todo
    }

    //! \brief Get a reference to given block's data. Block has to be already aquired.
    //! \param sblock Swapable block to access.
    block_data_holder_type & get_reference(SwapableBlockType * sblock) const
    { return sblock->get_reference(); }

    //! \brief Allocate a temporary swapable_block. Returns a pointer to the block.
    //! Temporary blocks are considered in prefetching. They may never be written to external memory if there is enough main memory.
    SwapableBlockType * allocate_temporary_swapable_block()
    {
        //todo
    }

    //! \brief Free given no longer used temporary swapable_block.
    //! \param sblock Temporary swapable_block to free.
    void free_temporary_swapable_block(SwapableBlockType * sblock)
    {
        //todo
    }

    //! \brief Turn on simulation mode and reset prediction sequence.
    void start_recording_prediction_sequence()
    {
        //todo
        //set dummy_internal_block
    }

    //! \brief Turn off simulation mode. Returns a reference to the recorded prediction sequence.
    const prediction_sequence_type & stop_recording_prediction_sequence()
    {
        //todo
    }

    //! \brief Turn on execute mode. Use the last used (or recorded) prediction sequence.
    void start_using_prediction_sequence()
    {
        //todo
    }

    //! \brief Turn on execute mode. Use the given prediction sequence.
    //! \param new_ps prediction sequence to copy and use.
    void start_using_prediction_sequence(const prediction_sequence_type & new_ps)
    {
        //todo
    }

    //! \brief Turn off execute mode.
    void stop_using_prediction_sequence()
    {
        //todo
    }

    //! \brief Returns if simulation mode is on, i.e. if a prediction sequence is beeing recorded.
    bool is_in_simulation_mode() const
    {
        //todo
        return false;
    }
};

// +-+-+-+-+-+-+ matrix version with swapable_blocks +-+-+-+-+-+-+-+-+-+-+-+-+-+

//! \brief Specialised swapable_block that interprets uninitiazized as containing zeros.
//! When initializing, all values are set to zero.
template <typename ValueType, unsigned BlockSideLength>
class matrix_swapable_block : public swapable_block<ValueType, BlockSideLength * BlockSideLength>
{
public:
    bool is_zero_block() const
    { return !this.is_initialized(); }

    void fill_with_zeros()
    { this.fill(0); }

    void fill_default()
    { fill_with_zeros(); }
};

//! \brief External container for the values of a matrix. Not intended for direct use.
template <typename ValueType, unsigned BlockSideLength>
struct matrix_data
{
    typedef matrix_swapable_block<ValueType, BlockSideLength> matrix_swapable_block_type;

    std::vector<matrix_swapable_block_type * > blocks;
    int_type height_in_blocks,
             width_in_blocks;
    bool transposed;

    matrix_swapable_block_type * get_block(int_type row, int_type col)
    {
        //todo
    }
};

//! \brief External matrix container.
template <typename ValueType, unsigned BlockSideLength>
class matrix
{
protected:
    typedef matrix_data<ValueType, BlockSideLength> matrix_data_type;

    int_type height,
             width;
    matrix_data_type data;

    //! \brief Creates a new matrix of given height and width. Elements' values are set to zero.
    //! \param new_height Height of the created matrix.
    //! \param new_width Width of the created matrix.
    matrix(int_type new_height, int_type new_width)
    {
        //todo
    }

    ~matrix()
    {
        //todo
    }

    //todo: standart container operations
    // []; elem(row, col); row- and col-iterator; get/set row/col; get/set submatrix
};

// +-+-+-+-+-+-+ blocked-matrix version +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

//forward declaration
template <typename ValueType, unsigned BlockSideLength>
class blocked_matrix;

//forward declaration for friend
template <typename ValueType, unsigned BlockSideLength>
blocked_matrix<ValueType, BlockSideLength> &
multiply(
    const blocked_matrix<ValueType, BlockSideLength> & A,
    const blocked_matrix<ValueType, BlockSideLength> & B,
    blocked_matrix<ValueType, BlockSideLength> & C,
    unsigned_type max_temp_mem_raw
    );


//! \brief External matrix container

//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam BlockSideLength side length of one square matrix block, default is 1024
//!         BlockSideLength*BlockSideLength*sizeof(ValueType) must be divisible by 4096
template <typename ValueType, unsigned BlockSideLength = 1024>
class blocked_matrix
{
    static const unsigned_type block_size = BlockSideLength * BlockSideLength;
    static const unsigned_type raw_block_size = block_size * sizeof(ValueType);

public:
    typedef typed_block<raw_block_size, ValueType> block_type;

private:
    typedef typename block_type::bid_type bid_type;
    typedef typename block_type::iterator element_iterator_type;

    bid_type * bids;
    block_manager * bm;
    const unsigned_type num_rows, num_cols;
    const unsigned_type num_block_rows, num_block_cols;
    MatrixBlockLayout * layout;

public:
    blocked_matrix(unsigned_type num_rows, unsigned_type num_cols, MatrixBlockLayout * given_layout = NULL)
        : num_rows(num_rows), num_cols(num_cols),
          num_block_rows(div_ceil(num_rows, BlockSideLength)),
          num_block_cols(div_ceil(num_cols, BlockSideLength)),
          layout((given_layout != NULL) ? given_layout : new RowMajor)
    {
        layout->set_dimensions(num_block_rows, num_block_cols);
        bm = block_manager::get_instance();
        bids = new bid_type[num_block_rows * num_block_cols];
        bm->new_blocks(striping(), bids, bids + num_block_rows * num_block_cols);
    }

    ~blocked_matrix()
    {
        bm->delete_blocks(bids, bids + num_block_rows * num_block_cols);
        delete[] bids;
        delete layout;
    }

    bid_type & bid(unsigned_type row, unsigned_type col) const
    {
        return *(bids + layout->coords_to_index(row, col));
    }

    //! \brief read in matrix from stream, assuming row-major order
    template <class InputIterator>
    void materialize_from_row_major(InputIterator & i, unsigned_type /*max_temp_mem_raw*/)
    {
        element_iterator_type current_element, first_element_of_row_in_block;

        // if enough space
        // allocate one row of blocks
        block_type * row_of_blocks = new block_type[num_block_cols];

        // iterate block-rows therein element-rows therein block-cols therein element-col
        // fill with elements from iterator rsp. padding with zeros
        for (unsigned_type b_row = 0; b_row < num_block_rows; ++b_row)
        {
            unsigned_type num_e_rows = (b_row < num_block_rows - 1)
                                       ? BlockSideLength : (num_rows - 1) % BlockSideLength + 1;
            // element-rows
            unsigned_type e_row;
            for (e_row = 0; e_row < num_e_rows; ++e_row)
                // block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block =
                        row_of_blocks[b_col].begin() + e_row * BlockSideLength;
                    unsigned_type num_e_cols = (b_col < num_block_cols - 1)
                                               ? BlockSideLength : (num_cols - 1) % BlockSideLength + 1;
                    // element-cols
                    for (current_element = first_element_of_row_in_block;
                         current_element < first_element_of_row_in_block + num_e_cols;
                         ++current_element)
                    {
                        // read element
                        //todo if (i.empty()) throw exception
                        *current_element = *i;
                        ++i;
                    }
                    // padding element-cols
                    for ( ; current_element < first_element_of_row_in_block + BlockSideLength;
                          ++current_element)
                        *current_element = 0;
                }
            // padding element-rows
            for ( ; e_row < BlockSideLength; ++e_row)
                // padding block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block =
                        row_of_blocks[b_col].begin() + e_row * BlockSideLength;
                    // padding element-cols
                    for (current_element = first_element_of_row_in_block;
                         current_element < first_element_of_row_in_block + BlockSideLength;
                         ++current_element)
                        *current_element = 0;
                }
            // write block-row to disk
            std::vector<request_ptr> requests;
            for (unsigned_type col = 0; col < num_block_cols; ++col)
                requests.push_back(row_of_blocks[col].write(bid(b_row, col)));
            wait_all(requests.begin(), requests.end());
        }
    }

    template <class OutputIterator>
    void output_to_row_major(OutputIterator & i, unsigned_type /*max_temp_mem_raw*/) const
    {
        element_iterator_type current_element, first_element_of_row_in_block;

        // if enough space
        // allocate one row of blocks
        block_type * row_of_blocks = new block_type[num_block_cols];

        // iterate block-rows therein element-rows therein block-cols therein element-col
        // write elements to iterator
        for (unsigned_type b_row = 0; b_row < num_block_rows; ++b_row)
        {
            // read block-row from disk
            std::vector<request_ptr> requests;
            for (unsigned_type col = 0; col < num_block_cols; ++col)
                requests.push_back(row_of_blocks[col].read(bid(b_row, col)));
            wait_all(requests.begin(), requests.end());

            unsigned_type num_e_rows = (b_row < num_block_rows - 1)
                                       ? BlockSideLength : (num_rows - 1) % BlockSideLength + 1;
            // element-rows
            unsigned_type e_row;
            for (e_row = 0; e_row < num_e_rows; ++e_row)
                // block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block =
                        row_of_blocks[b_col].begin() + e_row * BlockSideLength;
                    unsigned_type num_e_cols = (b_col < num_block_cols - 1)
                                               ? BlockSideLength : (num_cols - 1) % BlockSideLength + 1;
                    // element-cols
                    for (current_element = first_element_of_row_in_block;
                         current_element < first_element_of_row_in_block + num_e_cols;
                         ++current_element)
                    {
                        // write element
                        //todo if (i.empty()) throw exception
                        *i = *current_element;
                        ++i;
                    }
                }
        }
    }

    friend
    blocked_matrix<ValueType, BlockSideLength> &
    multiply<>(
        const blocked_matrix<ValueType, BlockSideLength> & A,
        const blocked_matrix<ValueType, BlockSideLength> & B,
        blocked_matrix<ValueType, BlockSideLength> & C,
        unsigned_type max_temp_mem_raw);
};

template <typename matrix_type>
class matrix_row_major_iterator
{
    typedef typename matrix_type::block_type block_type;
    typedef typename matrix_type::value_type value_type;

    matrix_type * matrix;
    block_type * row_of_blocks;
    bool * dirty;
    unsigned_type loaded_row_in_blocks,
        current_element;

public:
    matrix_row_major_iterator(matrix_type & m)
        : loaded_row_in_blocks(-1),
          current_element(0)
    {
        matrix = &m;
        // allocate one row of blocks
        row_of_blocks = new block_type[m.num_block_cols];
        dirty = new bool[m.num_block_cols];
    }

    matrix_row_major_iterator(const matrix_row_major_iterator& other)
    {
        matrix = other.matrix;
        row_of_blocks = NULL;
        dirty = NULL;
        loaded_row_in_blocks = -1;
        current_element = other.current_element;
    }

    matrix_row_major_iterator& operator=(const matrix_row_major_iterator& other)
    {

        matrix = other.matrix;
        row_of_blocks = NULL;
        dirty = NULL;
        loaded_row_in_blocks = -1;
        current_element = other.current_element;

        return *this;
    }

    ~matrix_row_major_iterator()
    {
        //TODO write out

        delete[] row_of_blocks;
        delete[] dirty;
    }

    matrix_row_major_iterator & operator ++ ()
    {
        ++current_element;
        return *this;
    }

    bool empty() const { return (current_element >= *matrix.num_rows * *matrix.num_cols); }

    value_type& operator * () { return 1 /*TODO*/; }

    const value_type& operator * () const { return 1 /*TODO*/; }
};

//! \brief submatrix of a matrix containing blocks (type block_type) that reside in main memory
template <typename matrix_type>
class panel
{
public:
    typedef typename matrix_type::block_type block_type;
    typedef typename block_type::iterator element_iterator_type;

    block_type * blocks;
    const RowMajor layout;
    unsigned_type height, width;

    panel(const unsigned_type max_height, const unsigned_type max_width)
        : layout(max_width, max_height),
          height(max_height), width(max_width)
    {
        blocks = new block_type[max_height * max_width];
    }

    ~panel()
    {
        delete[] blocks;
    }

    // fill the blocks specified by height and width with zeros
    void clear()
    {
        element_iterator_type elements;

        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // iterate elements
                for (elements = block(row, col).begin(); elements < block(row, col).end(); ++elements)
                    // set element zero
                    *elements = 0;
    }

    // read the blocks specified by height and width
    void read_sync(const matrix_type & from, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> requests = read_async(from, first_row, first_col);

        wait_all(requests.begin(), requests.end());
    }

    // read the blocks specified by height and width
    std::vector<request_ptr> &
    read_async(const matrix_type & from, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> * requests = new std::vector<request_ptr>; // todo is this the way to go?

        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // post request and save pointer
                (*requests).push_back(block(row, col).read(from.bid(first_row + row, first_col + col)));

        return *requests;
    }

    // write the blocks specified by height and width
    void write_sync(const matrix_type & to, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> requests = write_async(to, first_row, first_col);

        wait_all(requests.begin(), requests.end());
    }

    // read the blocks specified by height and width
    std::vector<request_ptr> &
    write_async(const matrix_type & to, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> * requests = new std::vector<request_ptr>; // todo is this the way to go?

        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // post request and save pointer
                (*requests).push_back(block(row, col).write(to.bid(first_row + row, first_col + col)));

        return *requests;
    }

    block_type & block(unsigned_type row, unsigned_type col) const
    {
        return *(blocks + layout.coords_to_index(row, col));
    }
};

#if STXXL_BLAS == 1
extern "C" void dgemm_(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
//TODO: cleanup variants
#elif STXXL_BLAS == 2
enum blas_order_type {
            blas_rowmajor = 101,
            blas_colmajor = 102 };
enum blas_trans_type {
            blas_no_trans   = 111,
            blas_trans      = 112,
            blas_conj_trans = 113 };
extern "C" void cblas_dgemm( enum blas_order_type order, enum blas_trans_type transa,
                 enum blas_trans_type transb, int m, int n, int k,
                 double alpha, const double *a, int lda, const double *b,
                 int ldb, double beta, double *c, int ldc );

#elif  STXXL_BLAS == 3
typedef int blas_int;
extern "C" void dgemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const double *alpha, const double *a, const blas_int *lda, const double *b, const blas_int *ldb,
        const double *beta, double *c, const blas_int *ldc);

//! \brief calculates c = alpha * a * b + beta * c
//! \param n height of a and c
//! \param k width of a and height of b
//! \param m width of b and c
//! \param a_in_col_major if a is stored in column-major rather than row-major
//! \param b_in_col_major if b is stored in column-major rather than row-major
//! \param c_in_col_major if c is stored in column-major rather than row-major
void dgemm_wrapper(const blas_int n, const blas_int k, const blas_int m,
        const double alpha, const bool a_in_col_major, const double *a /*, const blas_int lda*/,
                            const bool b_in_col_major, const double *b /*, const blas_int ldb*/,
        const double beta,  const bool c_in_col_major,       double *c /*, const blas_int ldc*/)
{
    const blas_int& stride_in_a = a_in_col_major ? n : k;
    const blas_int& stride_in_b = b_in_col_major ? k : m;
    const blas_int& stride_in_c = c_in_col_major ? n : m;
    const char transa = a_in_col_major xor c_in_col_major ? 'T' : 'N';
    const char transb = b_in_col_major xor c_in_col_major ? 'T' : 'N';
    if (c_in_col_major)
        // blas expects matrices in column-major unless specified via transa rsp. transb
        dgemm_(&transa, &transb, &n, &m, &k, &alpha, a, &stride_in_a, b, &stride_in_b, &beta, c, &stride_in_c);
    else
        // blas expects matrices in column-major, so we calulate c^T = alpha * b^T * a^T + beta * c^T
        dgemm_(&transb, &transa, &m, &n, &k, &alpha, b, &stride_in_b, a, &stride_in_a, &beta, c, &stride_in_c);
}
#endif

//! \brief multiplies matrices A and B, adds result to C
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <typename value_type, unsigned BlockSideLength>
struct low_level_multiply;

//! \brief multiplies matrices A and B, adds result to C, for double entries
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <unsigned BlockSideLength>
struct low_level_multiply<double, BlockSideLength>
{
    void operator () (double * a, double * b, double * c)
    {
    #if STXXL_BLAS == 1
        int n = BlockSideLength;
        char transpose = 'N';
        double alpha = 1.0;
        double beta = 1.0;

        dgemm_(&transpose, &transpose, &n, &n, &n, &alpha, b, &n, a, &n, &beta, c, &n);
    #elif STXXL_BLAS == 2
        cblas_dgemm(blas_rowmajor, blas_no_trans,
                blas_no_trans, BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a, BlockSideLength, b,
                BlockSideLength, 1.0, c, BlockSideLength);
    #elif STXXL_BLAS == 3
        dgemm_wrapper(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, false, a,
                     false, b,
                1.0, false, c);
    #else
        for (unsigned_type k = 0; k < BlockSideLength; ++k)
            #if STXXL_PARALLEL
            #pragma omp parallel for
            #endif
            for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                for (unsigned_type j = 0; j < BlockSideLength; ++j)
                    c[i * BlockSideLength + j] += a[i * BlockSideLength + k] * b[k * BlockSideLength + j];
    #endif
    }
};

template <typename value_type, unsigned BlockSideLength>
struct low_level_multiply
{
    void operator () (value_type * a, value_type * b, value_type * c)
    {
        for (unsigned_type k = 0; k < BlockSideLength; ++k)
            #if STXXL_PARALLEL
            #pragma omp parallel for
            #endif
            for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                for (unsigned_type j = 0; j < BlockSideLength; ++j)
                    c[i * BlockSideLength + j] += a[i * BlockSideLength + k] * b[k * BlockSideLength + j];
    }
};


//! \brief multiplies blocks of A and B, adds result to C
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <typename block_type, unsigned BlockSideLength>
void multiply_block(/*const*/ block_type & BlockA, /*const*/ block_type & BlockB, block_type & BlockC)
{
    typedef typename block_type::value_type value_type;

    value_type * a = BlockA.begin(), * b = BlockB.begin(), * c = BlockC.begin();
    low_level_multiply<value_type, BlockSideLength> llm;
    llm(a, b, c);
}

// multiply panels from A and B, add result to C
// param BlocksA pointer to first Block of A assumed in row-major
template <typename matrix_type, unsigned BlockSideLength>
void multiply_panel(const panel<matrix_type> & PanelA, const panel<matrix_type> & PanelB, panel<matrix_type> & PanelC)
{
    typedef typename matrix_type::block_type block_type;

    assert(PanelA.width == PanelB.height);
    assert(PanelC.height == PanelA.height);
    assert(PanelC.width == PanelB.width);

    for (unsigned_type row = 0; row < PanelC.height; ++row)
        for (unsigned_type col = 0; col < PanelC.width; ++col)
            for (unsigned_type l = 0; l < PanelA.width; ++l)
                multiply_block<block_type, BlockSideLength>(PanelA.block(row, l), PanelB.block(l, col), PanelC.block(row, col));
}

//! \brief multiply the matrices A and B, gaining C
template <typename ValueType, unsigned BlockSideLength>
blocked_matrix<ValueType, BlockSideLength> &
multiply(
    const blocked_matrix<ValueType, BlockSideLength> & A,
    const blocked_matrix<ValueType, BlockSideLength> & B,
    blocked_matrix<ValueType, BlockSideLength> & C,
    unsigned_type max_temp_mem_raw
    )
{
    typedef blocked_matrix<ValueType, BlockSideLength> matrix_type;
    typedef typename matrix_type::block_type block_type;

    assert(A.num_cols == B.num_rows);
    assert(C.num_rows == A.num_rows);
    assert(C.num_cols == B.num_cols);

    // preparation:
    // calculate panel size from blocksize and max_temp_mem_raw
    unsigned_type panel_max_side_length_in_blocks = sqrt(double(max_temp_mem_raw / 3 / block_type::raw_size));
    unsigned_type panel_max_num_n_in_blocks = panel_max_side_length_in_blocks, 
            panel_max_num_k_in_blocks = panel_max_side_length_in_blocks, 
            panel_max_num_m_in_blocks = panel_max_side_length_in_blocks,
            matrix_num_n_in_panels = div_ceil(C.num_block_rows, panel_max_num_n_in_blocks),
            matrix_num_k_in_panels = div_ceil(A.num_block_cols, panel_max_num_k_in_blocks),
            matrix_num_m_in_panels = div_ceil(C.num_block_cols, panel_max_num_m_in_blocks);
    /*  n, k and m denote the three dimensions of a matrix multiplication, according to the following ascii-art diagram:
     * 
     *                 +--m--+          
     *  +----k-----+   |     |   +--m--+
     *  |          |   |     |   |     |
     *  n    A     | • k  B  | = n  C  |
     *  |          |   |     |   |     |
     *  +----------+   |     |   +-----+
     *                 +-----+          
     */
    
    // reserve mem for a,b,c-panel
    panel<matrix_type> panelA(panel_max_num_n_in_blocks, panel_max_num_k_in_blocks);
    panel<matrix_type> panelB(panel_max_num_k_in_blocks, panel_max_num_m_in_blocks);
    panel<matrix_type> panelC(panel_max_num_n_in_blocks, panel_max_num_m_in_blocks);
    
    // multiply:
    // iterate rows and cols (panel wise) of c
    for (unsigned_type panel_row = 0; panel_row < matrix_num_n_in_panels; ++panel_row)
    {	//for each row
        panelC.height = panelA.height = (panel_row < matrix_num_n_in_panels -1) ?
                panel_max_num_n_in_blocks : /*last row*/ (C.num_block_rows-1) % panel_max_num_n_in_blocks +1;
        for (unsigned_type panel_col = 0; panel_col < matrix_num_m_in_panels; ++panel_col)
        {	//for each column

        	//for each panel of C
            panelC.width = panelB.width = (panel_col < matrix_num_m_in_panels -1) ?
                    panel_max_num_m_in_blocks : (C.num_block_cols-1) % panel_max_num_m_in_blocks +1;
            // initialize c-panel
            //TODO: acquire panelC (uninitialized)
            panelC.clear();
            // iterate a-row,b-col
            for (unsigned_type l = 0; l < matrix_num_k_in_panels; ++l)
            {	//scalar product over row/column
                panelA.width = panelB.height = (l < matrix_num_k_in_panels -1) ?
                        panel_max_num_k_in_blocks : (A.num_block_cols-1) % panel_max_num_k_in_blocks +1;
                // load a,b-panel
                //TODO: acquire panelA
                panelA.read_sync(A, panel_row*panel_max_num_n_in_blocks, l*panel_max_num_k_in_blocks);
                //TODO: acquire panelB
                panelB.read_sync(B, l * panel_max_num_k_in_blocks, panel_col * panel_max_num_m_in_blocks);
                // multiply and add to c
                multiply_panel<matrix_type, BlockSideLength>(panelA, panelB, panelC);
                //TODO: release panelA
                //TODO: release panelB
            }
            //TODO: release panelC (write)
            // write c-panel
            panelC.write_sync(C, panel_row * panel_max_num_n_in_blocks, panel_col * panel_max_num_m_in_blocks);
        }
    }

    return C;
}

__STXXL_END_NAMESPACE

#endif /* STXXL_MATRIX_HEADER */
// vim: et:ts=4:sw=4
