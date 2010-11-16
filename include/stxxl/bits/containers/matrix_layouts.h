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

#ifndef MATRIX_LAYOUTS_H_
#define MATRIX_LAYOUTS_H_

#include <stxxl/bits/namespace.h>

__STXXL_BEGIN_NAMESPACE

class RowMajor
{
	unsigned_type num_rows, num_cols;

public:
	RowMajor(unsigned_type num_rows, unsigned_type num_cols) : num_rows(num_rows), num_cols(num_cols) { }

	unsigned_type coords_to_index(unsigned_type row, unsigned_type col)
	{
		return row * num_cols + col;
	}

	std::pair<unsigned_type, unsigned_type> index_to_coords(unsigned_type index)
	{
		std::pair<unsigned_type, unsigned_type> coords(index / num_cols, index % num_cols);
		return coords;
	}
};

__STXXL_END_NAMESPACE

#endif /* MATRIX_LAYOUTS_H_ */
