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
	typedef typed_block<raw_block_size, ValueType> block_type;
	typedef typename block_type::bid_type bid_type;
	
	bid_type** blocks;
    block_manager* bm;
    unsigned_type num_rows, num_cols;
    unsigned_type num_block_rows, num_block_cols;
	Layout layout;
	
public:
	matrix(unsigned_type num_rows, unsigned_type num_cols)
		: num_rows(num_rows), num_cols(num_cols),
			num_block_rows(div_ceil(num_rows, BlockSideLength)),
			num_block_cols(div_ceil(num_cols, BlockSideLength)),
			layout(num_block_rows, num_block_cols)
	{
		
		std::vector<bid_type> bids(num_block_rows * num_block_cols);
        bm = block_manager::get_instance();
        bm->new_blocks(striping(), bids.begin(), bids.end(), 0);

		blocks = new bid_type*[num_block_rows];
		for (unsigned_type row = 0; row < num_block_rows; ++row)
		{
			blocks[row] = new bid_type[num_block_cols];
			for (unsigned_type col = 0; col < num_block_cols; ++col)
				blocks[row][col] = bids[layout.coords_to_index(row, col)];
		}
	}
	
	~matrix()
	{
		for (unsigned_type row = 0; row < num_block_rows; ++row)
		{
			bm->delete_blocks(blocks[row], blocks[row] + num_block_cols);
			delete[] blocks[row];
		}
		delete[] blocks;
	}
	
	
	//! \brief read in matrix from stream, assuming row-major order
	template<class InputIterator>
	void materialize_from_row_major(InputIterator& i)
	{
		while(!i.empty())
		{
			//TODO write materialize_from_row_major
			//do something with *i
			
			++i;
		}
	}
	
	//friend declaration
	template<typename SomeValueType, unsigned SomeBlockSideLength, class LayoutA, class LayoutB, class LayoutC>
	friend matrix<SomeValueType, SomeBlockSideLength, LayoutC>&
	multiply(
			const matrix<SomeValueType, SomeBlockSideLength, LayoutA>& A,
			const matrix<SomeValueType, SomeBlockSideLength, LayoutB>& B,
			matrix<SomeValueType, SomeBlockSideLength, LayoutC>& C,
			unsigned_type MaxTempMemRaw
			);
	
};


// a panel is a matrix containing blocks (type block_type) that resides in main memory
template<typename block_type>
struct panel
{
	block_type* blocks;
	const unsigned_type max_height, max_width;
	unsigned_type height, width;
	
	panel(const unsigned_type max_height, const unsigned_type max_width)
		: max_height(max_height),
			max_width(max_width)
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
		//todo code clear
	}
};

// multiplies blocks of A and B, adds result to C
// param pointer to block of A,B,C; elements in blocks have to be in row-major
template <typename block_type, unsigned BlockSideLength>
void multiply_block(const block_type& BlockA, const block_type& BlockB, block_type& BlockC)
{
        typedef typename block_type::value_type ValueType;
        // get pointers to data
        // TODO should  be block_type.iterator
        ValueType* AElements = BlockA.begin;
        ValueType* BElements = BlockB.begin;
        ValueType* CElements = BlockC.begin;
        for (unsigned_type row = 0; row < BlockSideLength; ++row)
                for (unsigned_type col = 0; col < BlockSideLength; ++col)
                        for (unsigned_type l = 0; l < BlockSideLength; ++l)
                                *(CElements + row*BlockSideLength + col) +=
                                                                *(AElements + row*BlockSideLength + l) * *(BElements + l*BlockSideLength + col);
}

// multiply panels from A and B, add result to C
// param BlocksA pointer to first Block of A assumed in row-major
template<typename block_type, unsigned BlockSideLength>
void multiply_panel(const panel<block_type>& PanelA, const panel<block_type>& PanelB, panel<block_type>& PanelC)
{
        assert(PanelA.width == PanelB.height);
        assert(PanelC.height == PanelA.height);
        assert(PanelC.width == PanelB.width);

        for (unsigned_type row = 0; row < PanelC.height; ++row)
                for (unsigned_type col = 0; col < PanelC.width; ++col)
                        for (unsigned_type l = 0; l < PanelA.width; ++l)
                                multiply_block(*(PanelA.blocks + row*PanelA.width + l),
                                                                *(PanelB.blocks + l*PanelB.width + col),
                                                                *(PanelC.blocks + row*PanelC.width + col));
}
	
//! \brief multiply the matrices A and B, gaining C
template<typename ValueType, unsigned BlockSideLength, class LayoutA, class LayoutB, class LayoutC>
matrix<ValueType, BlockSideLength, LayoutC>&
multiply(
		const matrix<ValueType, BlockSideLength, LayoutA>& A,
		const matrix<ValueType, BlockSideLength, LayoutB>& B,
		matrix<ValueType, BlockSideLength, LayoutC>& C,
		unsigned_type MaxTempMemRaw
		)
{
	typedef matrix<ValueType, BlockSideLength, LayoutA> MatrixA;
	typedef typename MatrixA::block_type block_type;

	/* // multiplies tiles of A and B, adds result to C
	// param pointer to tile of A,B,C; elements in tiles have to be in row-major
	void multiply_tile(ValueType* A, ValueType* B, ValueType* C)
	{
		for (unsigned_type row = 0; row < tileHeight; ++row)
			for (unsigned_type col = 0; col < tileWidth; ++col)
				for (unsigned_type l = 0; l < tileLength; ++l)
					*(C + row*tileWidth + col) += *(A + row*tileWidth + l) * *(B + l*tileLength + col);
  } */
	
	assert(A.num_cols == B.num_rows);
	assert(C.num_rows == A.num_rows);
	assert(C.num_cols == B.num_cols);

	//TODO multiply
	
	// preparation:
	// calculate panel size from blocksize and tempmem
	unsigned_type panel_max_block_side_length = sqrt(MaxTempMemRaw / 3 / sizeof(block_type));
	unsigned_type panel_max_block_num_1 = panel_max_block_side_length, 
					panel_max_block_num_2 = panel_max_block_side_length, 
					panel_max_block_num_3 = panel_max_block_side_length,
					num_panels_1 = div_ceil(C.num_block_rows, panel_max_block_num_1),
					num_panels_2 = div_ceil(A.num_block_cols, panel_max_block_num_2),
					num_panels_3 = div_ceil(C.num_block_cols, panel_max_block_num_3);
	// reserve mem for a,b,c-panel
	panel<block_type> panelA(panel_max_block_num_1,panel_max_block_num_2);
	panel<block_type> panelB(panel_max_block_num_2,panel_max_block_num_3);
	panel<block_type> panelC(panel_max_block_num_1,panel_max_block_num_3);
	
	// multiply:
	// iterate rows and cols (panel wise) of c
	for (unsigned_type row = 0; row < num_panels_1; ++row)
	{
		panelC.height  = panelA.height = 
				(row < num_panels_1 -1) ? panelC.max_height : (C.num_block_rows-1) % panelC.max_height +1;
		for (unsigned_type col = 0; col < num_panels_3; ++col)
		{
			panelC.width = panelB.width = 
					(col < num_panels_3 -1) ? panelC.max_width : (C.num_block_cols-1) % panelC.max_width +1;
			// initialize c-panel
			panelC.clear();
			// iterate a-row,b-col
			for (unsigned_type l = 0; l < num_panels_2; ++l)
			{
				panelA.width = panelB.height = 
						(l < num_panels_2 -1) ? panelA.max_width : (A.num_block_cols-1) % panelA.max_width +1;
				// load a,b-panel
				
				//for (;;)
					//request = *APanel.read(**A.blocks)
				// multiply and add to c  - subroutine: panel multiply (size)
				;
			}
				
			// write c-panel
		}
	}
	
	return C;
}

__STXXL_END_NAMESPACE

#endif /* STXXL_MATRIX_HEADER */








