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

//! \brief Interface for matrix block layout
class MatrixBlockLayout
{
protected:
    unsigned_type num_rows, num_cols;

public:
    MatrixBlockLayout() { }
    MatrixBlockLayout(unsigned_type num_rows, unsigned_type num_cols)
        : num_rows(num_rows), num_cols(num_cols) { }

    virtual void set_dimensions(unsigned_type _num_rows, unsigned_type _num_cols)
    {
        num_rows = _num_rows;
        num_cols = _num_cols;
    }

    virtual unsigned_type coords_to_index(unsigned_type row, unsigned_type col) const = 0;
    virtual std::pair<unsigned_type, unsigned_type> index_to_coords(unsigned_type index) const = 0;
};

//! \brief Row-major block mapping {0,..,num_rows-1}x{0,..,num_cols-1} <-> {0,..,num_rows*num_cols-1}
class RowMajor : public MatrixBlockLayout
{
public:
    RowMajor() { }
    RowMajor(unsigned_type num_rows, unsigned_type num_cols) : MatrixBlockLayout(num_rows, num_cols) { }

    virtual unsigned_type coords_to_index(unsigned_type row, unsigned_type col) const
    {
        return row * num_cols + col;
    }

    virtual std::pair<unsigned_type, unsigned_type> index_to_coords(unsigned_type index) const
    {
        return std::make_pair(index / num_cols, index % num_cols);     //(row, col)
    }
};

//! \brief Column-major block mapping {0,..,num_rows-1}x{0,..,num_cols-1} <-> {0,..,num_rows*num_cols-1}
class ColumnMajor : public MatrixBlockLayout
{
public:
    ColumnMajor() { }
    ColumnMajor(unsigned_type num_rows, unsigned_type num_cols) : MatrixBlockLayout(num_rows, num_cols) { }

    virtual unsigned_type coords_to_index(unsigned_type row, unsigned_type col) const
    {
        return col * num_rows + row;
    }

    virtual std::pair<unsigned_type, unsigned_type> index_to_coords(unsigned_type index) const
    {
        return std::make_pair(index % num_rows, index / num_rows);     //(row, col)
    }
};

__STXXL_END_NAMESPACE

#endif /* MATRIX_LAYOUTS_H_ */
// vim: et:ts=4:sw=4
