/***************************************************************************
 *  include/stxxl/bits/containers/matrix.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MATRIX_HEADER
#define STXXL_MATRIX_HEADER

#ifndef STXXL_BLAS
#define STXXL_BLAS 0
#endif

//#include <stxxl/bits/mng/mng.h>
//#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/containers/matrix_layouts.h>
#include <stxxl/bits/mng/block_scheduler.h>


__STXXL_BEGIN_NAMESPACE

// +-+-+-+-+-+-+ matrix version with swappable_blocks +-+-+-+-+-+-+-+-+-+-+-+-+-+

/* index-variable naming convention:
 * [MODIFIER_][UNIT_]DIMENSION[_in_[MODIFIER_]ENVIRONMENT]
 *
 * e.g.:
 * block_row = number of row measured in rows consisting of blocks
 * element_row_in_block = number of row measured in rows consisting of elements in the (row of) block(s)
 *
 * size-variable naming convention:
 * [MODIFIER_][ENVIRONMENT_]DIMENSION[_in_UNITs]
 *
 * e.g.
 * height_in_blocks
 */

template <typename ValueType, unsigned BlockSideLength> class matrix;

//! \brief Specialised swappable_block that interprets uninitiazized as containing zeros.
//! When initializing, all values are set to zero.
template <typename ValueType, unsigned BlockSideLength>
class matrix_swappable_block : public swappable_block<ValueType, BlockSideLength * BlockSideLength>
{
    using swappable_block<ValueType, BlockSideLength * BlockSideLength>::fill;
public:
    void fill_default()
    { fill(0); }
};

//! \brief External container for the values of a (sub)matrix. Not intended for direct use.
//!
//! Stores blocks only, so all measures (height, width, row, col) are in blocks.
template <typename ValueType, unsigned BlockSideLength>
class swappable_block_matrix : private noncopyable
{
public:
    typedef int_type size_type;
    typedef int_type index_type;
    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef block_scheduler< matrix_swappable_block<ValueType, BlockSideLength> > block_scheduler_type;
    typedef typename block_scheduler_type::swappable_block_identifier_type swappable_block_identifier_type;

    block_scheduler_type & bs;

    //! \brief height of the matrix in blocks
    const size_type height,
    //! \brief width of the matrix in blocks
                    width,
    //! \brief height copied from supermatrix
                    height_from_supermatrix,
    //! \brief width copied from supermatrix
                    width_from_supermatrix;
protected:
    //! \brief the matrice's blocks in row-major
    std::vector<swappable_block_identifier_type> blocks;
    //! \brief if the elements in each block are in col-major instead of row-major
    bool elements_in_blocks_transposed;

    swappable_block_identifier_type & block(index_type row, index_type col)
    { return blocks[row * width + col]; }

public:
    //! \brief Create an empty swappable_block_matrix of given dimensions.
    swappable_block_matrix(block_scheduler_type & bs, const size_type height_in_blocks, const size_type width_in_blocks)
        : bs(bs),
          height(height_in_blocks),
          width(width_in_blocks),
          height_from_supermatrix(0),
          width_from_supermatrix(0),
          blocks(height * width),
          elements_in_blocks_transposed(false)
    {
        for (index_type row = 0; row < height; ++row)
            for (index_type col = 0; col < width; ++col)
                block(row, col) = bs.allocate_swappable_block();
    }

    //! \brief Create swappable_block_matrix of given dimensions that
    //!        represents the submatrix of supermatrix starting at (row_in_blocks, col_in_blocks).
    //!
    //! If supermatrix is not large enough, the submatrix is padded with empty blocks.
    //! The supermatrix must not be destructed before the submatrix.
    swappable_block_matrix(swappable_block_matrix_type & supermatrix,
            const size_type height_in_blocks, const size_type width_in_blocks,
            const index_type from_row_in_blocks, const index_type from_col_in_blocks)
        : bs(supermatrix.bs),
          height(height_in_blocks),
          width(width_in_blocks),
          height_from_supermatrix(std::min(supermatrix.height - from_row_in_blocks, height)),
          width_from_supermatrix(std::min(supermatrix.width - from_col_in_blocks, width)),
          blocks(height * width),
          elements_in_blocks_transposed(supermatrix.elements_in_blocks_transposed)
    {
        for (index_type row = 0; row < height_from_supermatrix; ++row)
        {
            for (index_type col = 0; col < width_from_supermatrix; ++col)
                block(row, col) = supermatrix.block(row + from_row_in_blocks, col + from_col_in_blocks);
            for (index_type col = width_from_supermatrix; col < width; ++col)
                block(row, col) = bs.allocate_swappable_block();
        }
        for (index_type row = height_from_supermatrix; row < height; ++row)
            for (index_type col = 0; col < width; ++col)
                block(row, col) = bs.allocate_swappable_block();
    }

    ~swappable_block_matrix()
    {
        for (index_type row = 0; row < height_from_supermatrix; ++row)
        {
            for (index_type col = width_from_supermatrix; col < width; ++col)
                bs.free_swappable_block(block(row, col));
        }
        for (index_type row = height_from_supermatrix; row < height; ++row)
            for (index_type col = 0; col < width; ++col)
                bs.free_swappable_block(block(row, col));
    }

    const swappable_block_identifier_type operator () (index_type row, index_type col)
    { return block(row, col); }

    bool is_transposed()
    { return elements_in_blocks_transposed; }

    void transpose()
    {
        elements_in_blocks_transposed = ! elements_in_blocks_transposed;
        //todo transpose blocks
    }

    void set_zero(); // deinitialize all blocks
};

template <typename ValueType, unsigned BlockSideLength>
class matrix_iterator
{
protected:
    typedef matrix_iterator<ValueType, BlockSideLength> matrix_iterator_type;
    typedef matrix<ValueType, BlockSideLength> matrix_type;
    typedef typename matrix_type::swappable_block_matrix_type swappable_block_matrix_type;
    typedef typename matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename matrix_type::elem_size_type elem_size_type;
    typedef typename matrix_type::elem_index_type elem_index_type;
    typedef typename matrix_type::block_size_type block_size_type;
    typedef typename matrix_type::block_index_type block_index_type;

    template <typename VT, unsigned BSL> friend class matrix;

    matrix_type & m;
    swappable_block_matrix_type & mdata;
    block_scheduler_type & bs;
    elem_index_type current_row, // \ both indices == -1 <=> empty iterator
                    current_col; // /
    block_index_type current_block_row,
                     current_block_col;
    internal_block_type * current_iblock; // NULL if block is not acquired

    void acquire_current_iblock()
    {
        if (! current_iblock)
            current_iblock = & bs.acquire(mdata(current_block_row, current_block_col));
    }

    void release_current_iblock()
    {
        if (current_iblock)
            bs.release(mdata(current_block_row, current_block_col), true);
    }

    //! \brief create iterator pointing to given row and col
    matrix_iterator(matrix_type & matrix, const elem_index_type start_row, const elem_index_type start_col)
        : m(matrix),
          mdata(*matrix.data),
          bs(mdata.bs),
          current_row(start_row),
          current_col(start_col),
          current_iblock(0) {}

    //! \brief create empty iterator
    matrix_iterator(matrix_type & matrix)
        : m(matrix),
          mdata(*matrix.data),
          bs(mdata.bs),
          current_row(-1), // empty iterator
          current_col(-1),
          current_iblock(0) {}

    ~matrix_iterator()
    {
        release_current_iblock();
    }

    void set_empty()
    {
        current_row = -1;
        current_col = -1;
    }
public:
    matrix_iterator(const matrix_iterator_type & other)
        : m(other.m),
          mdata(other.mdata),
          bs(other.bs),
          current_row(other.current_row),
          current_col(other.current_col),
          current_iblock(0)
    {
        if (other.current_iblock)
            acquire_current_iblock();
    }

    matrix_iterator_type & operator = (const matrix_iterator_type & other)
    {
        assert(&m == &other.m);
        set_coordinates(other.current_row, other.current_col);
        if (other.current_iblock)
            acquire_current_iblock();
        return *this;
    }

    void set_row(const elem_index_type new_row)
    {
        const block_index_type new_block_row = m.block_index_from_elem(new_row);
        if (new_block_row != current_block_row)
        {
            release_current_iblock();
            current_block_row = new_block_row;
        }
        current_row = new_row;
    }

    void set_col(const elem_index_type new_col)
    {
        const block_index_type new_block_col = m.block_index_from_elem(new_col);
        if (new_block_col != current_block_col)
        {
            release_current_iblock();
            current_block_col = new_block_col;
        }
        current_col = new_col;
    }

    void set_coordinates(const elem_index_type new_row, const elem_index_type new_col)
    {
        const block_index_type new_block_row = m.block_index_from_elem(new_row),
                new_block_col = m.block_index_from_elem(new_col);
        if (new_block_col != current_block_col || new_block_row != current_block_row)
        {
            release_current_iblock();
            current_block_row = new_block_row;
            current_block_col = new_block_col;
        }
        current_row = new_row;
        current_col = new_col;
    }

    void set_coordinates(const std::pair<elem_index_type, elem_index_type> new_coordinates)
    { set_coordinates(new_coordinates.first, new_coordinates.second); }

    elem_index_type get_row() const
    { return current_row; }

    elem_index_type get_col() const
    { return current_col; }

    std::pair<elem_index_type, elem_index_type> get_coordinates() const
    { return std::make_pair(current_row, current_col); }

    bool empty() const
    { return current_row == -1 && current_col == -1; }

    operator bool () const
    { return ! empty(); }

    bool operator == (const matrix_iterator_type & other) const
    {
        assert(&m == &other.m);
        return current_row == other.current_row && current_col == other.current_col;
    }

    // do not store reference
    ValueType & operator * ()
    {
        acquire_current_iblock();
        return (*current_iblock)[m.elem_index_in_block_from_elem(current_row, current_col)];
    }
};

template <typename ValueType, unsigned BlockSideLength>
class matrix_row_major_iterator : public matrix_iterator<ValueType, BlockSideLength>
{
protected:
    typedef matrix_iterator<ValueType, BlockSideLength> matrix_iterator_type;
    typedef matrix_row_major_iterator<ValueType, BlockSideLength> matrix_row_major_iterator_type;
    typedef typename matrix_iterator_type::matrix_type matrix_type;
    typedef typename matrix_iterator_type::elem_index_type elem_index_type;

    template <typename VT, unsigned BSL> friend class matrix;

    using matrix_iterator_type::m;
    using matrix_iterator_type::get_row;
    using matrix_iterator_type::get_col;
    using matrix_iterator_type::set_col;
    using matrix_iterator_type::set_coordinates;
    using matrix_iterator_type::set_empty;

    //! \brief create iterator pointing to given row and col
    matrix_row_major_iterator(matrix_type & matrix, const elem_index_type start_row, const elem_index_type start_col)
        : matrix_iterator_type(matrix, start_row, start_col) {}

    //! \brief create empty iterator
    matrix_row_major_iterator(matrix_type & matrix)
    : matrix_iterator_type(matrix) {}

public:
    // Has to be not empty, else behavior is undefined.
    matrix_row_major_iterator_type & operator ++ ()
    {
        if (get_col() + 1 < m.width)
            // => not at the end of row, move right
            set_col(get_col() + 1);
        else if (get_row() + 1 < m.height)
            // => at end of row but not last row, move to beginning of next row
            set_coordinates(get_row() + 1, 0);
        else
            // => at end of matrix, set to empty-state
            set_empty();
        return *this;
    }

    // Has to be not empty, else behavior is undefined.
    matrix_row_major_iterator_type & operator -- ()
    {
        if (get_col() - 1 >= 0)
            // => not at the beginning of row, move left
            set_col(get_col() - 1);
        else if (get_row() - 1 >= 0)
            // => at beginning of row but not first row, move to end of previous row
            set_coordinates(get_row() - 1, m.width - 1);
        else
            // => at beginning of matrix, set to empty-state
            set_empty();
        return *this;
    }
};

template <typename ValueType, unsigned BlockSideLength>
class matrix_col_major_iterator : public matrix_iterator<ValueType, BlockSideLength>
{
protected:
    typedef matrix_iterator<ValueType, BlockSideLength> matrix_iterator_type;
    typedef matrix_col_major_iterator<ValueType, BlockSideLength> matrix_col_major_iterator_type;
    typedef typename matrix_iterator_type::matrix_type matrix_type;
    typedef typename matrix_iterator_type::elem_index_type elem_index_type;

    template <typename VT, unsigned BSL> friend class matrix;

    using matrix_iterator_type::m;
    using matrix_iterator_type::get_row;
    using matrix_iterator_type::get_col;
    using matrix_iterator_type::set_row;
    using matrix_iterator_type::set_coordinates;
    using matrix_iterator_type::set_empty;

    //! \brief create iterator pointing to given row and col
    matrix_col_major_iterator(matrix_type & matrix, const elem_index_type start_row, const elem_index_type start_col)
        : matrix_iterator_type(matrix, start_row, start_col) {}

    //! \brief create empty iterator
    matrix_col_major_iterator(matrix_type & matrix)
        : matrix_iterator_type(matrix) {}

public:
    // Has to be not empty, else behavior is undefined.
    matrix_col_major_iterator_type & operator ++ ()
    {
        if (get_row() + 1 < m.height)
            // => not at the end of col, move down
            set_row(get_row() + 1);
        else if (get_col() + 1 < m.width)
            // => at end of col but not last col, move to beginning of next col
            set_coordinates(0, get_col() + 1);
        else
            // => at end of matrix, set to empty-state
            set_empty();
        return *this;
    }

    // Has to be not empty, else behavior is undefined.
    matrix_col_major_iterator_type & operator -- ()
    {
        if (get_row() - 1 >= 0)
            // => not at the beginning of col, move up
            set_row(get_row() - 1);
        else if (get_col() - 1 >= 0)
            // => at beginning of col but not first col, move to end of previous col
            set_coordinates(m.height - 1, get_col() - 1);
        else
            // => at beginning of matrix, set to empty-state
            set_empty();
        return *this;
    }
};

/*template <typename ValueType, unsigned BlockSideLength>
class matrix_reference : private matrix_iterator<ValueType, BlockSideLength>
{
    typedef matrix_reference<ValueType, BlockSideLength> matrix_reference_type;

    using matrix_iterator<ValueType, BlockSideLength>:: operator *;
public:
    operator ValueType () const
    { return operator * (); }

    matrix_reference_type & operator = (const ValueType new_value)
    {
        operator * () = new_value;
        return *this;
    }
}; //*/

//! \brief External matrix container.
template <typename ValueType, unsigned BlockSideLength>
class matrix : private noncopyable
{
protected:
    typedef int_type elem_size_type;
    typedef int_type elem_index_type;
    typedef int_type elem_index_in_block_type;
    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename swappable_block_matrix_type::size_type block_size_type;
    typedef typename swappable_block_matrix_type::index_type block_index_type;

public:
    typedef matrix_row_major_iterator<ValueType, BlockSideLength> row_major_iterator;
    typedef matrix_col_major_iterator<ValueType, BlockSideLength> col_major_iterator;
    typedef row_major_iterator iterator;

protected:
    template <typename VT, unsigned BSL> friend class matrix_iterator;

public:
    const elem_size_type height,
                         width;
protected:
    swappable_block_matrix_type * data;

    static block_index_type block_index_from_elem(elem_index_type index)
    { return index / BlockSideLength; }

    static elem_index_in_block_type elem_index_in_block_from_elem(elem_index_type index)
    { return index % BlockSideLength; }

    // takes care about transposed
    elem_index_in_block_type elem_index_in_block_from_elem(elem_index_type row, elem_index_type col)
    {
        return (data->is_transposed())
                 ? row % BlockSideLength + col % BlockSideLength * BlockSideLength
                 : row % BlockSideLength * BlockSideLength + col % BlockSideLength;
    }

public:
    //! \brief Creates a new matrix of given dimensions. Elements' values are set to zero.
    //! \param height height of the created matrix
    //! \param width width of the created matrix
    matrix(block_scheduler_type & bs, const elem_size_type height, const elem_size_type width)
        : height(height),
          width(width),
          data(new swappable_block_matrix_type
                  (bs, div_ceil(height, BlockSideLength), div_ceil(width, BlockSideLength)))
    {}

    ~matrix()
    {
        delete data;
    }

    row_major_iterator begin()
    { return row_major_iterator(*this, 0, 0); }

    row_major_iterator end()
    { return row_major_iterator(*this); }

    row_major_iterator begin_row_major()
    { return row_major_iterator(*this, 0, 0); }

    row_major_iterator end_row_major()
    { return row_major_iterator(*this); }

    col_major_iterator begin_col_major()
    { return col_major_iterator(*this, 0, 0); }

    col_major_iterator end_col_major()
    { return col_major_iterator(*this); }

    // do not store reference
    ValueType & operator () (elem_index_type row, elem_index_type col)
    { return *iterator(*this, row, col); }

    //todo: standart container operations
    // get/set row/col; get/set submatrix

    //maydo: col-major iterator, cheap iterator
};

/*  n, m and l denote the three dimensions of a matrix multiplication, according to the following ascii-art diagram:
 *
 *                 +--m--+
 *  +----l-----+   |     |   +--m--+
 *  |          |   |     |   |     |
 *  n    A     | • l  B  | = n  C  |
 *  |          |   |     |   |     |
 *  +----------+   |     |   +-----+
 *                 +-----+
 *
 * The index-variables are called i, j, k for dimension
 *                                n, m, l .
 */

template <typename SwappableBlockMatrixType>
struct matrix_multiply
{

};

template <typename ValueType, unsigned BlockSideLength>
struct swappable_block_matrix_approximative_quarterer
{
    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;

    swappable_block_matrix_type  upleft,   upright,
                                downleft, downright,
            & ul, & ur, & dl, & dr;

    swappable_block_matrix_approximative_quarterer(swappable_block_matrix_type & whole)
        : upleft   (whole,                whole.height/2,               whole.width/2,              0,             0),
          upright  (whole,                whole.height/2, whole.width - whole.width/2,              0, whole.width/2),
          downleft (whole, whole.height - whole.height/2,               whole.width/2, whole.height/2,             0),
          downright(whole, whole.height - whole.height/2, whole.width - whole.width/2, whole.height/2, whole.width/2),
          ul(upleft), ur(upright), dl(downleft), dr(downright) {}
};

//! \brief calculates C = A * B + C
// requires fitting dimensions
template <typename ValueType, unsigned BlockSideLength>
swappable_block_matrix<ValueType, BlockSideLength> &
multyply_and_add_recursive(
            const swappable_block_matrix<ValueType, BlockSideLength> A,
            const swappable_block_matrix<ValueType, BlockSideLength> B,
            swappable_block_matrix<ValueType, BlockSideLength> C)
{
    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::index_type index_type;
    typedef typename swappable_block_matrix_type::size_type size_type;

    typedef swappable_block_matrix_approximative_quarterer<ValueType, BlockSideLength>
            approximative_quarterer_type;

    // todo remove assertion after securing earlyer
    assert(C.height == A.height && C.width == B.width && A.width == B.height);

    // base case
    if (C.height == 1 || C.width == 1 || A.width == 1)
        return multiply_and_add_naive(A, B, C);

    // partition matrix
    approximative_quarterer_type * qa = new approximative_quarterer_type(A),
                                 * qb = new approximative_quarterer_type(B),
                                 * qc = new approximative_quarterer_type(C);
    // recursive multiplication
    multyply_and_add_recursive(qa.ul, qb.ul, qc.ul);
    multyply_and_add_recursive(qa.ur, qb.dl, qc.ul);
    multyply_and_add_recursive(qa.ul, qb.ur, qc.ur);
    multyply_and_add_recursive(qa.ur, qb.dr, qc.ur);
    multyply_and_add_recursive(qa.dl, qb.ul, qc.dl);
    multyply_and_add_recursive(qa.dr, qb.dl, qc.dl);
    multyply_and_add_recursive(qa.dl, qb.ur, qc.dr);
    multyply_and_add_recursive(qa.dr, qb.dr, qc.dr);
    // delete partitioning
    delete qa, qb, qc;
}

//! \brief multiplies matrices A and B, adds result to C
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <typename ValueType, unsigned BlockSideLength>
struct block_multiply;

//! \brief calculates C = A * B + C
// requires fitting dimensions
template <typename ValueType, unsigned BlockSideLength>
swappable_block_matrix<ValueType, BlockSideLength> &
multyply_and_add_naive(
            const swappable_block_matrix<ValueType, BlockSideLength> A,
            const swappable_block_matrix<ValueType, BlockSideLength> B,
            swappable_block_matrix<ValueType, BlockSideLength> C)
{
    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::swappable_block_identifier_type swappable_block_identifier_type;
    typedef typename swappable_block_matrix_type::index_type index_type;
    typedef typename swappable_block_matrix_type::size_type size_type;

    const size_type & n = C.height,
                      m = C.width,
                      l = A.width;
    for (index_type i = 0; i < n; ++i)
        for (index_type j = 0; j < m; ++j)
            for (index_type k = 0; k < l; ++k)
            {
                const swappable_block_identifier_type a = A(i,k),
                                                      b = B(k,j);
                swappable_block_identifier_type c = C(i,j);
                // calculate c = a * b + c
                block_multiply<ValueType, BlockSideLength>::mult(a,b,c);
            }
}

template <typename ValueType, unsigned BlockSideLength>
swappable_block_matrix<ValueType, BlockSideLength> &
multyply_and_add_swappable_block(
            const swappable_block_matrix<ValueType, BlockSideLength> A,
            const swappable_block_matrix<ValueType, BlockSideLength> B,
            swappable_block_matrix<ValueType, BlockSideLength> C)
{

}

#if STXXL_BLAS
typedef int blas_int;
extern "C" void dgemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const double *alpha, const double *a, const blas_int *lda, const double *b, const blas_int *ldb,
        const double *beta, double *c, const blas_int *ldc);

//! \brief calculates c = alpha * a * b + beta * c
//! \param n height of a and c
//! \param l width of a and height of b
//! \param m width of b and c
//! \param a_in_col_major if a is stored in column-major rather than row-major
//! \param b_in_col_major if b is stored in column-major rather than row-major
//! \param c_in_col_major if c is stored in column-major rather than row-major
void dgemm_wrapper(const blas_int n, const blas_int l, const blas_int m,
        const double alpha, const bool a_in_col_major, const double *a /*, const blas_int lda*/,
                            const bool b_in_col_major, const double *b /*, const blas_int ldb*/,
        const double beta,  const bool c_in_col_major,       double *c /*, const blas_int ldc*/)
{
    const blas_int& stride_in_a = a_in_col_major ? n : l;
    const blas_int& stride_in_b = b_in_col_major ? l : m;
    const blas_int& stride_in_c = c_in_col_major ? n : m;
    const char transa = a_in_col_major xor c_in_col_major ? 'T' : 'N';
    const char transb = b_in_col_major xor c_in_col_major ? 'T' : 'N';
    if (c_in_col_major)
        // blas expects matrices in column-major unless specified via transa rsp. transb
        dgemm_(&transa, &transb, &n, &m, &l, &alpha, a, &stride_in_a, b, &stride_in_b, &beta, c, &stride_in_c);
    else
        // blas expects matrices in column-major, so we calulate c^T = alpha * b^T * a^T + beta * c^T
        dgemm_(&transb, &transa, &m, &n, &l, &alpha, b, &stride_in_b, a, &stride_in_a, &beta, c, &stride_in_c);
}

//! \brief multiplies matrices A and B, adds result to C, for double entries
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <unsigned BlockSideLength>
struct block_multiply<double, BlockSideLength>
{
    typedef swappable_block_matrix<double, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::swappable_block_identifier_type swappable_block_identifier_type;

    block_multiply(const swappable_block_identifier_type a,
                   const swappable_block_identifier_type b,
                   swappable_block_identifier_type c)
    {
        const bool & a_in_col_major,
                   & b_in_col_major,
                   & c_in_col_major;
        const double * pa,
                     * pb;
        double * pc;
        dgemm_wrapper(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a_in_col_major, pa,
                     b_in_col_major, pb,
                1.0, c_in_col_major, pc);
    }

    static void mult (const bool a_in_col_major, const double * a,
                      const bool b_in_col_major, const double * b,
                      const bool c_in_col_major, double * c)
    {
        dgemm_wrapper(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a_in_col_major, a,
                     b_in_col_major, b,
                1.0, c_in_col_major, c);
    }
};
#endif

template <typename ValueType, unsigned BlockSideLength>
struct block_multiply
{
    static void mult (const bool a_in_col_major, const double * a,
                      const bool b_in_col_major, const double * b,
                      const bool c_in_col_major, double * c)
    {
        #if STXXL_PARALLEL
        #pragma omp parallel for
        #endif
        for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
            for (unsigned_type k = 0; k < BlockSideLength; ++k)
                for (unsigned_type j = 0; j < BlockSideLength; ++j)
                    c[i * BlockSideLength + j] += a[i * BlockSideLength + k] * b[k * BlockSideLength + j];
    }
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
class blocked_matrix_row_major_iterator
{
    typedef typename matrix_type::block_type block_type;
    typedef typename matrix_type::value_type value_type;

    matrix_type * matrix;
    block_type * row_of_blocks;
    bool * dirty;
    unsigned_type loaded_row_in_blocks,
        current_element;

public:
    blocked_matrix_row_major_iterator(matrix_type & m)
        : loaded_row_in_blocks(-1),
          current_element(0)
    {
        matrix = &m;
        // allocate one row of blocks
        row_of_blocks = new block_type[m.num_block_cols];
        dirty = new bool[m.num_block_cols];
    }

    blocked_matrix_row_major_iterator(const blocked_matrix_row_major_iterator& other)
    {
        matrix = other.matrix;
        row_of_blocks = NULL;
        dirty = NULL;
        loaded_row_in_blocks = -1;
        current_element = other.current_element;
    }

    blocked_matrix_row_major_iterator& operator=(const blocked_matrix_row_major_iterator& other)
    {

        matrix = other.matrix;
        row_of_blocks = NULL;
        dirty = NULL;
        loaded_row_in_blocks = -1;
        current_element = other.current_element;

        return *this;
    }

    ~blocked_matrix_row_major_iterator()
    {
        //TODO write out

        delete[] row_of_blocks;
        delete[] dirty;
    }

    blocked_matrix_row_major_iterator & operator ++ ()
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
    #if STXXL_BLAS
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
