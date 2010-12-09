/***************************************************************************
 *  include/stxxl/bits/containers/matrix_layouts.h
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

#include <utility>
#include <stxxl/bits/common/types.h>


__STXXL_BEGIN_NAMESPACE

// capsules a mapping {0,..,num_rows-1}x{0,..,num_cols-1} <-> {0,..,num_rows*num_cols-1}
class RowMajor
{
    const unsigned_type num_rows, num_cols;

public:
    RowMajor(unsigned_type num_rows, unsigned_type num_cols)
        : num_rows(num_rows), num_cols(num_cols) { }

    unsigned_type coords_to_index(unsigned_type row, unsigned_type col) const
    {
        return row * num_cols + col;
    }

    std::pair<unsigned_type, unsigned_type> index_to_coords(unsigned_type index) const
    {
        std::pair<unsigned_type, unsigned_type> coords(index / num_cols, index % num_cols);
        return coords;
    }
};

__STXXL_END_NAMESPACE

#endif /* MATRIX_LAYOUTS_H_ */
// vim: et:ts=4:sw=4
