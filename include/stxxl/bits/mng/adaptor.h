#ifndef ADAPTOR_HEADER
#define ADAPTOR_HEADER

/***************************************************************************
 *            adaptor.h
 *
 *  Sat Aug 24 23:55:18 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include <iterator>

#include "stxxl/bits/common/types.h"


__STXXL_BEGIN_NAMESPACE

//! \addtogroup mnglayer
//!
//! \{

template < class Tp_, class Distance_ > struct r_a_iterator
{
    typedef std::random_access_iterator_tag iterator_category;
    typedef Tp_ value_type;
    typedef Distance_ difference_type;
    typedef Tp_ * pointer;
    typedef Tp_ & reference;
};

#define STXXL_ADAPTOR_ARITHMETICS(pos) \
    bool operator == (const _Self & a) const \
    { \
        return (a.pos == pos); \
    }; \
    bool operator != (const _Self & a) const \
    { \
        return (a.pos != pos); \
    }; \
    bool operator < (const _Self & a) const \
    { \
        return (pos < a.pos); \
    }; \
    bool operator > (const _Self & a) const \
    { \
        return (pos > a.pos); \
    }; \
    bool operator <= (const _Self & a) const \
    { \
        return (pos <= a.pos); \
    }; \
    bool operator >= (const _Self & a) const \
    { \
        return (pos >= a.pos); \
    }; \
    _Self operator + (pos_type off) const \
    { \
        return _Self (array, pos + off); \
    } \
    _Self operator - (pos_type off) const \
    { \
        return _Self (array, pos - off); \
    } \
    _Self & operator ++ () \
    { \
        pos++; \
        return *this; \
    } \
    _Self operator ++ (int)  \
    { \
        _Self tmp = *this; \
        pos++; \
        return tmp; \
    } \
    _Self & operator -- () \
    { \
        pos--; \
        return *this; \
    } \
    _Self operator -- (int) \
    { \
        _Self tmp = *this; \
        pos--; \
        return tmp; \
    } \
    pos_type operator - (const _Self & a) const \
    { \
        return pos - a.pos; \
    } \
    _Self & operator -= (pos_type off) \
    { \
        pos -= off; \
        return *this; \
    } \
    _Self & operator += (pos_type off) \
    { \
        pos += off; \
        return *this; \
    }

template < class one_dim_array_type, class data_type,
          class pos_type >
struct TwoToOneDimArrayAdaptorBase : public r_a_iterator <
                                                          data_type, pos_type >
{
    one_dim_array_type * array;
    pos_type pos;
    typedef pos_type _pos_type;
    typedef TwoToOneDimArrayAdaptorBase < one_dim_array_type,
                                         data_type, pos_type > _Self;



    TwoToOneDimArrayAdaptorBase ()
    { };

    TwoToOneDimArrayAdaptorBase (one_dim_array_type * a, pos_type p) : array (a),
                                                                       pos(p)
    { };
    TwoToOneDimArrayAdaptorBase (const TwoToOneDimArrayAdaptorBase &a) : array (a.array),
                                                                         pos (a.pos)
    { };

    STXXL_ADAPTOR_ARITHMETICS(pos)
};


//////////////////////////////

#define BLOCK_ADAPTOR_OPERATORS(two_to_one_dim_array_adaptor_base)              \
                                                                                \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                 \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> & operator ++ (  \
        two_to_one_dim_array_adaptor_base < _blk_sz, _run_type, __pos_type > & a)              \
    {                                                                               \
        a.pos++;                                                                      \
        return a;                                                                     \
    }                                                                               \
                                                                                \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                 \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator ++ (    \
        two_to_one_dim_array_adaptor_base < _blk_sz, _run_type, __pos_type > & a, int)         \
    {                                                                               \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> tmp = a;              \
        a.pos++;                                                                      \
        return tmp;                                                                   \
    }                                                                               \
                                                                                \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                 \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> & operator -- (  \
        two_to_one_dim_array_adaptor_base < _blk_sz, _run_type, __pos_type > & a)              \
    {                                                                               \
        a.pos--;                                                                      \
        return a;                                                                     \
    }                                                                               \
                                                                                \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                 \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator -- (    \
        two_to_one_dim_array_adaptor_base < _blk_sz, _run_type, __pos_type > & a, int)         \
    {                                                                               \
        two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> tmp = a;                      \
        a.pos--;                                                                              \
        return tmp;                                                                           \
    }                                                                                       \
                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                         \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> & operator -= (          \
        two_to_one_dim_array_adaptor_base < _blk_sz, _run_type, __pos_type > & a, \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)          \
    {                                                                                       \
        a.pos -= off;                                                                           \
        return a;                                                                             \
    }                                                                                       \
                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                         \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> & operator += (          \
        two_to_one_dim_array_adaptor_base < _blk_sz, _run_type, __pos_type > & a, \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)          \
    {                                                                                       \
        a.pos += off;                                                                           \
        return a;                                                                             \
    }                                                                                       \
                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                         \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator + (             \
        const two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> & a, \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)           \
    {                                                                                       \
        return two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>(a.array, a.pos + off);  \
    }                                                                                       \
                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                         \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator + (             \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off, \
        const two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> & a)                        \
    {                                                                                       \
        return two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>(a.array, a.pos + off);  \
    }                                                                                       \
                                                                                        \
    template <unsigned _blk_sz, typename _run_type, class __pos_type>                                         \
    inline two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> operator - (             \
        const two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type> & a, \
        typename two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>::_pos_type off)          \
    {                                                                                       \
        return two_to_one_dim_array_adaptor_base<_blk_sz, _run_type, __pos_type>(a.array, a.pos - off);  \
    }


//////////////////////////
template < class one_dim_array_type, class data_type,
          unsigned dim_size, class pos_type =
              int_type >struct TwoToOneDimArrayRowAdaptor : public
TwoToOneDimArrayAdaptorBase < one_dim_array_type, data_type,
                             pos_type >
{
    typedef TwoToOneDimArrayRowAdaptor < one_dim_array_type,
                                        data_type, dim_size, pos_type > _Self;

    typedef TwoToOneDimArrayAdaptorBase < one_dim_array_type,
                                         data_type, pos_type > _Parent;
    using _Parent::array;
    using _Parent::pos;

    TwoToOneDimArrayRowAdaptor ()
    { };
    TwoToOneDimArrayRowAdaptor (one_dim_array_type * a,
                                pos_type
                                p) : TwoToOneDimArrayAdaptorBase <
                                                                  one_dim_array_type, data_type, pos_type > (a, p)
    { };
    TwoToOneDimArrayRowAdaptor (const TwoToOneDimArrayRowAdaptor &
                                a) : TwoToOneDimArrayAdaptorBase <
                                                                  one_dim_array_type, data_type, pos_type > (a)
    { };

    data_type & operator * ()
    {
        return array[(pos) / dim_size][(pos) % dim_size];
    };

    data_type * operator -> () const
    {
        return &(array[(pos) / dim_size][(pos) % dim_size]);
    };

    data_type & operator [](pos_type n)
    {
        n += pos;
        return array[(n) / dim_size][(n) % dim_size];
    };
    STXXL_ADAPTOR_ARITHMETICS(pos)
};

template < class one_dim_array_type, class data_type,
          unsigned dim_size, class pos_type =
              int_type >struct TwoToOneDimArrayColumnAdaptor : public
TwoToOneDimArrayAdaptorBase < one_dim_array_type, data_type,
                             pos_type >
{
    typedef TwoToOneDimArrayColumnAdaptor < one_dim_array_type,
                                           data_type, dim_size, pos_type > _Self;

    using TwoToOneDimArrayAdaptorBase < one_dim_array_type, data_type, pos_type >::pos;
    using TwoToOneDimArrayAdaptorBase < one_dim_array_type, data_type, pos_type >::array;

    TwoToOneDimArrayColumnAdaptor (one_dim_array_type * a,
                                   pos_type
                                   p) : TwoToOneDimArrayAdaptorBase
                                        < one_dim_array_type, data_type, pos_type > (a, p)
    { };
    TwoToOneDimArrayColumnAdaptor (const _Self &
                                   a) : TwoToOneDimArrayAdaptorBase
                                        < one_dim_array_type, data_type, pos_type > (a)
    { };

    data_type & operator * ()
    {
        return array[(pos) % dim_size][(pos) / dim_size];
    };

    data_type * operator -> () const
    {
        return &(array[(pos) % dim_size][(pos) / dim_size]);
    };

    const data_type & operator [] (pos_type n) const
    {
        n += pos;
        return array[(n) % dim_size][(n) / dim_size];
    };

    data_type & operator [](pos_type n)
    {
        n += pos;
        return array[(n) % dim_size][(n) / dim_size];
    };
    STXXL_ADAPTOR_ARITHMETICS(pos)
};

//! \}

__STXXL_END_NAMESPACE

#endif
