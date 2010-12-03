/***************************************************************************
 *  include/stxxl/bits/containers/matrix.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MATRIX_HEADER
#define STXXL_MATRIX_HEADER

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/containers/matrix_layouts.h>

__STXXL_BEGIN_NAMESPACE

//! \brief External matrix container

//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam BlockSideLength side length of one square matrix block, default is 1024
//!         BlockSideLength*BlockSideLength*sizeof(ValueType) must be divisible by 4096
//! \tparam Layout layout in which the blocks are ordered on disk among each other
template<typename ValueType, unsigned BlockSideLength = 1024, class Layout = RowMajor>
class matrix
{
    static const unsigned_type block_size = BlockSideLength * BlockSideLength;
    static const unsigned_type raw_block_size = block_size * sizeof(ValueType);
public:
    //typedef typename matrix<ValueType, BlockSideLength, Layout> matrix_type;
    typedef typed_block<raw_block_size, ValueType> block_type;
private:
    typedef typename block_type::bid_type bid_type;
    typedef typename block_type::iterator element_iterator_type;
    
    bid_type* bids;
    block_manager* bm;
    const unsigned_type num_rows, num_cols;
    const unsigned_type num_block_rows, num_block_cols;
    const Layout layout;
    
public:
    matrix(unsigned_type num_rows, unsigned_type num_cols)
        : num_rows(num_rows), num_cols(num_cols),
            num_block_rows(div_ceil(num_rows, BlockSideLength)),
            num_block_cols(div_ceil(num_cols, BlockSideLength)),
            layout(num_block_rows, num_block_cols)
    {
        bm = block_manager::get_instance();
        bids = new bid_type[num_block_rows * num_block_cols];
        bm->new_blocks(striping(), bids, bids + num_block_rows * num_block_cols);
    }
    
    ~matrix()
    {
        bm->delete_blocks(bids, bids + num_block_rows * num_block_cols);
        delete[] bids;
    }
    
    bid_type& bid(unsigned_type row, unsigned_type col) const
    {
        return *(bids + layout.coords_to_index(row, col));
        int foo;  // this line is here to cause a compiler warning, to see if warnings are shown by the IDE 
    }
    
    //! \brief read in matrix from stream, assuming row-major order
    template<class InputIterator>
    void materialize_from_row_major(InputIterator& i, unsigned_type /*max_temp_mem_raw*/)
    {
        element_iterator_type current_element, first_element_of_row_in_block;
        
        // if enough space
        // allocate one row of blocks (panel ?)
        block_type* row_of_blocks = new block_type[num_block_cols];
        
        // iterate block-rows therein element-rows therein block-cols therein element-col
        // fill with elements from iterator rsp. padding with zeros 
        for (unsigned_type b_row = 0; b_row < num_block_rows; ++b_row)
        {
            unsigned_type num_e_rows = (b_row < num_block_rows -1) 
                    ? BlockSideLength : num_rows % BlockSideLength;
            unsigned_type e_row;
            // element-rows
            for (e_row = 0; e_row < num_e_rows; ++e_row)
                // block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block = 
                            row_of_blocks[b_col].begin() + e_row*BlockSideLength;
                    unsigned_type num_e_cols = (b_col < num_block_cols -1) 
                            ? BlockSideLength : num_cols % BlockSideLength;
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
                    for (; current_element < first_element_of_row_in_block + BlockSideLength; 
                            ++current_element)
                        *current_element = 0;
                }
            // padding element-rows
            for (; e_row < BlockSideLength; ++e_row)
                // padding block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block = 
                            row_of_blocks[b_col].begin() + e_row*BlockSideLength;
                    // padding element-cols
                    for (current_element = first_element_of_row_in_block; 
                            current_element < first_element_of_row_in_block + BlockSideLength; 
                            ++current_element)
                        *current_element = 0;
                }
            //write block-row to disk
            std::vector<request_ptr> requests;
            for (unsigned_type col = 0; col < num_block_cols; ++col)
                requests.push_back(row_of_blocks[col].write(bid(b_row, col)));
            wait_all(requests.data(), requests.size());
        }
    }
    
    //friend declaration
    template<typename SomeValueType, unsigned SomeBlockSideLength, class SomeLayout>
    friend matrix<SomeValueType, SomeBlockSideLength, SomeLayout>&
    multiply(
            const matrix<SomeValueType, SomeBlockSideLength, SomeLayout>& A,
            const matrix<SomeValueType, SomeBlockSideLength, SomeLayout>& B,
            matrix<SomeValueType, SomeBlockSideLength, SomeLayout>& C,
            unsigned_type max_temp_mem_raw);
};


// a panel is a matrix containing blocks (type block_type) that resides in main memory
template <typename matrix_type, class Layout = RowMajor>
class panel
{ 
public:
    typedef typename matrix_type::block_type block_type;
    typedef typename block_type::iterator element_iterator_type;
    
    block_type* blocks;
    const Layout layout;
    unsigned_type height, width;
    
    panel(const unsigned_type max_height, const unsigned_type max_width)
        : layout(max_height, max_width),
          height(max_height), width(max_width)
    {
        blocks = new block_type[max_height*max_width];
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
    void read_sync(const matrix_type& from, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> requests = read_async(from, first_row, first_col);
        
        wait_all(requests.data(), requests.size());
    }
    
    // read the blocks specified by height and width
    std::vector<request_ptr>& 
    read_async(const matrix_type& from, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr>* requests = new std::vector<request_ptr>; // todo is this the way to go?
        
        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // post request and save pointer
                (*requests).push_back(block(row, col).read(from.bid(first_row + row, first_col + col)));
        
        return *requests;
    }
    
    // write the blocks specified by height and width
    void write_sync(const matrix_type& to, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> requests = write_async(to, first_row, first_col);
        
        wait_all(requests.data(), requests.size());
    }
    
    // read the blocks specified by height and width
    std::vector<request_ptr>& 
    write_async(const matrix_type& to, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr>* requests = new std::vector<request_ptr>; // todo is this the way to go?
        
        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // post request and save pointer
                (*requests).push_back(block(row, col).write(to.bid(first_row + row, first_col + col)));
        
        return *requests;
    }
    
    block_type& block(unsigned_type row, unsigned_type col) const
    {
        return *(blocks + layout.coords_to_index(row, col));
    }
};

// multiplies blocks of A and B, adds result to C
// param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <typename block_type, unsigned BlockSideLength>
void multiply_block(/*const*/ block_type& BlockA, /*const*/ block_type& BlockB, block_type& BlockC)
{
    for (unsigned_type row = 0; row < BlockSideLength; ++row)
        for (unsigned_type col = 0; col < BlockSideLength; ++col)
            for (unsigned_type l = 0; l < BlockSideLength; ++l)
                BlockC[row*BlockSideLength + col] += 
                        BlockA[row*BlockSideLength + l] * BlockB[l*BlockSideLength + col];
}

// multiply panels from A and B, add result to C
// param BlocksA pointer to first Block of A assumed in row-major
template<typename matrix_type, unsigned BlockSideLength>
void multiply_panel(const panel<matrix_type>& PanelA, const panel<matrix_type>& PanelB, panel<matrix_type>& PanelC)
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
template<typename ValueType, unsigned BlockSideLength, class Layout>
matrix<ValueType, BlockSideLength, Layout>&
multiply(
        const matrix<ValueType, BlockSideLength, Layout>& A,
        const matrix<ValueType, BlockSideLength, Layout>& B,
        matrix<ValueType, BlockSideLength, Layout>& C,
        unsigned_type max_temp_mem_raw
)
{
    typedef matrix<ValueType, BlockSideLength, Layout> matrix_type;
    typedef typename matrix_type::block_type block_type;
    
    assert(A.num_cols == B.num_rows);
    assert(C.num_rows == A.num_rows);
    assert(C.num_cols == B.num_cols);
    
    // preparation:
    // calculate panel size from blocksize and tempmem
    unsigned_type panel_max_block_side_length = sqrt(max_temp_mem_raw / 3 / sizeof(block_type));
    unsigned_type panel_max_block_num_1 = panel_max_block_side_length, 
            panel_max_block_num_2 = panel_max_block_side_length, 
            panel_max_block_num_3 = panel_max_block_side_length,
            num_panels_1 = div_ceil(C.num_block_rows, panel_max_block_num_1),
            num_panels_2 = div_ceil(A.num_block_cols, panel_max_block_num_2),
            num_panels_3 = div_ceil(C.num_block_cols, panel_max_block_num_3);
    // reserve mem for a,b,c-panel
    panel<matrix_type> panelA(panel_max_block_num_1,panel_max_block_num_2);
    panel<matrix_type> panelB(panel_max_block_num_2,panel_max_block_num_3);
    panel<matrix_type> panelC(panel_max_block_num_1,panel_max_block_num_3);
    
    // multiply:
    // iterate rows and cols (panel wise) of c
    for (unsigned_type row = 0; row < num_panels_1; ++row)
    {
        panelC.height  = panelA.height = (row < num_panels_1 -1) ? 
                panel_max_block_num_1 : (C.num_block_rows-1) % panel_max_block_num_1 +1;
        for (unsigned_type col = 0; col < num_panels_3; ++col)
        {
            panelC.width = panelB.width = (col < num_panels_3 -1) ? 
                    panel_max_block_num_3 : (C.num_block_cols-1) % panel_max_block_num_3 +1;
            // initialize c-panel
            panelC.clear();
            // iterate a-row,b-col
            for (unsigned_type l = 0; l < num_panels_2; ++l)
            {
                panelA.width = panelB.height = (l < num_panels_2 -1) ? 
                        panel_max_block_num_2 : (A.num_block_cols-1) % panel_max_block_num_2 +1;
                // load a,b-panel
                panelA.read_sync(A, row*panel_max_block_num_1, l*panel_max_block_num_2);
                panelB.read_sync(B, l * panel_max_block_num_2, col * panel_max_block_num_3);
                // multiply and add to c
                multiply_panel<matrix_type, BlockSideLength>(panelA, panelB, panelC);
            }
            // write c-panel
            panelC.write_sync(C, row * panel_max_block_num_1, col * panel_max_block_num_3);
        }
    }
    
    return C;
}

__STXXL_END_NAMESPACE

#endif /* STXXL_MATRIX_HEADER */









