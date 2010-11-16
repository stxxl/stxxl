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
		: 	num_rows(num_rows), num_cols(num_cols),
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
};

__STXXL_END_NAMESPACE

#endif /* STXXL_MATRIX_HEADER */
