/***************************************************************************
 *  include/stxxl/bits/mng/adaptor.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2009-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_ADAPTOR_HEADER
#define STXXL_MNG_ADAPTOR_HEADER

#include <iterator>

#include <stxxl/bits/common/types.h>


STXXL_BEGIN_NAMESPACE

//! \addtogroup mnglayer
//!
//! \{


template <unsigned_type modulo>
class blocked_index
{
    unsigned_type pos;
    unsigned_type block;
    unsigned_type offset;

    //! \invariant block * modulo + offset = pos

    void set(unsigned_type pos)
    {
        this->pos = pos;
        block = pos / modulo;
        offset = pos % modulo;
    }

public:
    blocked_index()
    {
        set(0);
    }

    blocked_index(unsigned_type pos)
    {
        set(pos);
    }

    blocked_index(unsigned_type block, unsigned_type offset)
    {
        this->block = block;
        this->offset = offset;
        pos = block * modulo + offset;
    }

    void operator = (unsigned_type pos)
    {
        set(pos);
    }

    //pre-increment operator
    blocked_index& operator ++ ()
    {
        ++pos;
        ++offset;
        if (offset == modulo)
        {
            offset = 0;
            ++block;
        }
        return *this;
    }

    //post-increment operator
    blocked_index operator ++ (int)
    {
        blocked_index former(*this);
        operator ++ ();
        return former;
    }

    //pre-increment operator
    blocked_index& operator -- ()
    {
        --pos;
        if (offset == 0)
        {
            offset = modulo;
            --block;
        }
        --offset;
        return *this;
    }

    //post-increment operator
    blocked_index operator -- (int)
    {
        blocked_index former(*this);
        operator -- ();
        return former;
    }

    blocked_index& operator += (unsigned_type addend)
    {
        set(pos + addend);
        return *this;
    }

    blocked_index& operator >>= (unsigned_type shift)
    {
        set(pos >> shift);
        return *this;
    }

    operator unsigned_type () const
    {
        return pos;
    }

    const unsigned_type & get_block() const
    {
        return block;
    }

    const unsigned_type & get_offset() const
    {
        return offset;
    }
};

#define STXXL_ADAPTOR_ARITHMETICS(pos)         \
    bool operator == (const _Self& a) const    \
    {                                          \
        return (a.pos == pos);                 \
    }                                          \
    bool operator != (const _Self& a) const    \
    {                                          \
        return (a.pos != pos);                 \
    }                                          \
    bool operator < (const _Self& a) const     \
    {                                          \
        return (pos < a.pos);                  \
    }                                          \
    bool operator > (const _Self& a) const     \
    {                                          \
        return (pos > a.pos);                  \
    }                                          \
    bool operator <= (const _Self& a) const    \
    {                                          \
        return (pos <= a.pos);                 \
    }                                          \
    bool operator >= (const _Self& a) const    \
    {                                          \
        return (pos >= a.pos);                 \
    }                                          \
    _Self operator + (pos_type off) const      \
    {                                          \
        return _Self(array, pos + off);        \
    }                                          \
    _Self operator - (pos_type off) const      \
    {                                          \
        return _Self(array, pos - off);        \
    }                                          \
    _Self& operator ++ ()                      \
    {                                          \
        pos++;                                 \
        return *this;                          \
    }                                          \
    _Self operator ++ (int)                    \
    {                                          \
        _Self tmp = *this;                     \
        pos++;                                 \
        return tmp;                            \
    }                                          \
    _Self& operator -- ()                      \
    {                                          \
        pos--;                                 \
        return *this;                          \
    }                                          \
    _Self operator -- (int)                    \
    {                                          \
        _Self tmp = *this;                     \
        pos--;                                 \
        return tmp;                            \
    }                                          \
    pos_type operator - (const _Self& a) const \
    {                                          \
        return pos - a.pos;                    \
    }                                          \
    _Self& operator -= (pos_type off)          \
    {                                          \
        pos -= off;                            \
        return *this;                          \
    }                                          \
    _Self& operator += (pos_type off)          \
    {                                          \
        pos += off;                            \
        return *this;                          \
    }

template <class one_dim_array_type, class data_type, class pos_type>
struct two2one_dim_array_adapter_base
    : public std::iterator<std::random_access_iterator_tag, data_type, unsigned_type>
{
    one_dim_array_type* array;
    pos_type pos;
    typedef pos_type _pos_type;
    typedef two2one_dim_array_adapter_base<one_dim_array_type,
                                           data_type, pos_type> _Self;


    two2one_dim_array_adapter_base()
    { }

    two2one_dim_array_adapter_base(one_dim_array_type* a, pos_type p)
        : array(a), pos(p)
    { }
    two2one_dim_array_adapter_base(const two2one_dim_array_adapter_base& a)
        : array(a.array), pos(a.pos)
    { }

    STXXL_ADAPTOR_ARITHMETICS(pos)
};


//////////////////////////////

#define BLOCK_ADAPTOR_OPERATORS(two_to_one_dim_array_adaptor_base)                                      \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& operator ++ (             \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a)                           \
    {                                                                                                   \
        a.pos++;                                                                                        \
        return a;                                                                                       \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator ++ (              \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a, int)                      \
    {                                                                                                   \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> tmp = a;                      \
        a.pos++;                                                                                        \
        return tmp;                                                                                     \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& operator -- (             \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a)                           \
    {                                                                                                   \
        a.pos--;                                                                                        \
        return a;                                                                                       \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator -- (              \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a, int)                      \
    {                                                                                                   \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> tmp = a;                      \
        a.pos--;                                                                                        \
        return tmp;                                                                                     \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& operator -= (             \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a,                           \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)      \
    {                                                                                                   \
        a.pos -= off;                                                                                   \
        return a;                                                                                       \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& operator += (             \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a,                           \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)      \
    {                                                                                                   \
        a.pos += off;                                                                                   \
        return a;                                                                                       \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator + (               \
        const two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a,                     \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)      \
    {                                                                                                   \
        return two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>(a.array, a.pos + off); \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator + (               \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off,      \
        const two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a)                     \
    {                                                                                                   \
        return two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>(a.array, a.pos + off); \
    }                                                                                                   \
                                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                   \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator - (               \
        const two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>& a,                     \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)      \
    {                                                                                                   \
        return two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>(a.array, a.pos - off); \
    }


#if 0
//////////////////////////
template <class one_dim_array_type, class data_type,
          unsigned dim_size, class pos_type = blocked_index<dim_size> >
struct two2one_dim_array_row_adapter :
    public two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>
{
    typedef two2one_dim_array_row_adapter<one_dim_array_type,
                                          data_type, dim_size, pos_type> _Self;

    typedef two2one_dim_array_adapter_base<one_dim_array_type,
                                           data_type, pos_type> _Parent;
    using _Parent::array;
    using _Parent::pos;

    two2one_dim_array_row_adapter()
    { }
    two2one_dim_array_row_adapter(one_dim_array_type* a, pos_type p)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a, p)
    { }
    two2one_dim_array_row_adapter(const two2one_dim_array_row_adapter& a)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a)
    { }

    data_type& operator * ()
    {
        return array[(pos).get_block()][(pos).get_offset()];
    }

    data_type* operator -> () const
    {
        return &(array[(pos).get_block()][(pos).get_offset()]);
    }

    data_type& operator [] (pos_type n)
    {
        n += pos;
        return array[(n) / dim_size][(n) % dim_size];
    }

    const data_type& operator [] (pos_type n) const
    {
        n += pos;
        return array[(n) / dim_size][(n) % dim_size];
    }
    STXXL_ADAPTOR_ARITHMETICS(pos)
};

template <class one_dim_array_type, class data_type,
          unsigned dim_size, class pos_type = blocked_index<dim_size> >
struct two2one_dim_array_column_adapter
    : public two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>
{
    typedef two2one_dim_array_column_adapter<one_dim_array_type,
                                             data_type, dim_size, pos_type> _Self;

    using two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>::pos;
    using two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>::array;

    two2one_dim_array_column_adapter(one_dim_array_type* a, pos_type p)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a, p)
    { }
    two2one_dim_array_column_adapter(const _Self& a)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a)
    { }

    data_type& operator * ()
    {
        return array[(pos).get_offset()][(pos).get_block()];
    }

    data_type* operator -> () const
    {
        return &(array[(pos).get_offset()][(pos).get_block()]);
    }

    const data_type& operator [] (pos_type n) const
    {
        n += pos;
        return array[(n) % dim_size][(n) / dim_size];
    }

    data_type& operator [] (pos_type n)
    {
        n += pos;
        return array[(n) % dim_size][(n) / dim_size];
    }
    STXXL_ADAPTOR_ARITHMETICS(pos)
};
#endif


template <typename array_type, typename value_type, unsigned_type modulo>
class array_of_sequences_iterator : public std::iterator<std::random_access_iterator_tag, value_type, unsigned_type>
{
    unsigned_type pos;
    unsigned_type offset;
    array_type* arrays;
    array_type* base;
    value_type* base_element;

    //! \invariant block * modulo + offset = pos

    void set(unsigned_type pos)
    {
        this->pos = pos;
        offset = pos % modulo;
        base = arrays + pos / modulo;
        base_element = base->elem;
    }

public:
    array_of_sequences_iterator()
    {
        this->arrays = NULL;
        set(0);
    }

    array_of_sequences_iterator(array_type* arrays)
    {
        this->arrays = arrays;
        set(0);
    }

    array_of_sequences_iterator(array_type* arrays, unsigned_type pos)
    {
        this->arrays = arrays;
        set(pos);
    }

    void operator = (unsigned_type pos)
    {
        set(pos);
    }

    //pre-increment operator
    array_of_sequences_iterator& operator ++ ()
    {
        ++pos;
        ++offset;
        if (offset == modulo)
        {
            offset = 0;
            ++base;
            base_element = base->elem;
        }
        return *this;
    }

    //post-increment operator
    array_of_sequences_iterator operator ++ (int)
    {
        array_of_sequences_iterator former(*this);
        operator ++ ();
        return former;
    }

    //pre-increment operator
    array_of_sequences_iterator& operator -- ()
    {
        --pos;
        if (offset == 0)
        {
            offset = modulo;
            --base;
            base_element = base->elem;
        }
        --offset;
        return *this;
    }

    //post-increment operator
    array_of_sequences_iterator operator -- (int)
    {
        array_of_sequences_iterator former(*this);
        operator -- ();
        return former;
    }

    array_of_sequences_iterator& operator += (unsigned_type addend)
    {
        set(pos + addend);
        return *this;
    }

    array_of_sequences_iterator& operator -= (unsigned_type addend)
    {
        set(pos - addend);
        return *this;
    }

    array_of_sequences_iterator operator + (unsigned_type addend) const
    {
        return array_of_sequences_iterator(arrays, pos + addend);
    }

    array_of_sequences_iterator operator - (unsigned_type subtrahend) const
    {
        return array_of_sequences_iterator(arrays, pos - subtrahend);
    }

    unsigned_type operator - (const array_of_sequences_iterator& subtrahend) const
    {
        return pos - subtrahend.pos;
    }

    bool operator == (const array_of_sequences_iterator& aoai) const
    {
        return pos == aoai.pos;
    }

    bool operator != (const array_of_sequences_iterator& aoai) const
    {
        return pos != aoai.pos;
    }

    bool operator < (const array_of_sequences_iterator& aoai) const
    {
        return pos < aoai.pos;
    }

    bool operator <= (const array_of_sequences_iterator& aoai) const
    {
        return pos <= aoai.pos;
    }

    bool operator > (const array_of_sequences_iterator& aoai) const
    {
        return pos > aoai.pos;
    }

    bool operator >= (const array_of_sequences_iterator& aoai) const
    {
        return pos >= aoai.pos;
    }

    const value_type& operator * () const
    {
        return base_element[offset];
    }

    value_type& operator * ()
    {
        return base_element[offset];
    }

    const value_type& operator -> () const
    {
        return &(base_element[offset]);
    }

    value_type& operator -> ()
    {
        return &(base_element[offset]);
    }

    const value_type& operator [] (unsigned_type index) const
    {
        return arrays[index / modulo][index % modulo];
    }

    value_type& operator [] (unsigned_type index)
    {
        return arrays[index / modulo][index % modulo];
    }
};

namespace helper {

template <typename BlockType, bool can_use_trivial_pointer>
class element_iterator_generator
{ };

// default case for blocks with fillers or other data: use array_of_sequences_iterator
template <typename BlockType>
class element_iterator_generator<BlockType, false>
{
    typedef BlockType block_type;
    typedef typename block_type::value_type value_type;

public:
    typedef array_of_sequences_iterator<block_type, value_type, block_type::size> iterator;

    iterator operator () (block_type* blocks, unsigned_type offset) const
    {
        return iterator(blocks, offset);
    }
};

// special case for completely filled blocks: use trivial pointers
template <typename BlockType>
class element_iterator_generator<BlockType, true>
{
    typedef BlockType block_type;
    typedef typename block_type::value_type value_type;

public:
    typedef value_type* iterator;

    iterator operator () (block_type* blocks, unsigned_type offset) const
    {
        return blocks[0].elem + offset;
    }
};

} // namespace helper

template <typename BlockType>
struct element_iterator_traits
{
    typedef typename helper::element_iterator_generator<BlockType, BlockType::has_only_data>::iterator element_iterator;
};

template <typename BlockType>
inline
typename element_iterator_traits<BlockType>::element_iterator
make_element_iterator(BlockType* blocks, unsigned_type offset)
{
    helper::element_iterator_generator<BlockType, BlockType::has_only_data> iter_gen;
    return iter_gen(blocks, offset);
}

//! \}

STXXL_END_NAMESPACE

#endif // !STXXL_MNG_ADAPTOR_HEADER
// vim: et:ts=4:sw=4
