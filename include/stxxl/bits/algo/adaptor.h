#ifndef ALGO_ADAPTOR_HEADER
#define ALGO_ADAPTOR_HEADER

/***************************************************************************
 *            adaptor.h
 *
 *  Fri Oct  4 23:28:04 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/mng/mng.h"
#include "stxxl/bits/mng/adaptor.h"
#include "stxxl/bits/algo/interleaved_alloc.h"


__STXXL_BEGIN_NAMESPACE

template < unsigned _blk_sz, typename _run_type, class __pos_type = int_type >
struct RunsToBIDArrayAdaptor : public TwoToOneDimArrayAdaptorBase < _run_type *, BID < _blk_sz > , __pos_type >
{
    typedef RunsToBIDArrayAdaptor < _blk_sz, _run_type, __pos_type > _Self;
    typedef BID < _blk_sz > data_type;

    enum    { block_size = _blk_sz };

    unsigned_type dim_size;

    typedef TwoToOneDimArrayAdaptorBase < _run_type *, BID < _blk_sz >,
                                         __pos_type > _Parent;
    using _Parent::array;
    using _Parent::pos;

    RunsToBIDArrayAdaptor (_run_type * *a, __pos_type p,
                           unsigned_type d) : TwoToOneDimArrayAdaptorBase <_run_type *, BID < _blk_sz >, __pos_type > (a, p), dim_size (d)
    { };
    RunsToBIDArrayAdaptor (const _Self &a) : TwoToOneDimArrayAdaptorBase <_run_type *, BID < _blk_sz >, __pos_type > (a),
                                             dim_size (a.dim_size)
    { };

    const _Self & operator = (const _Self & a)
    {
        array = a.array;
        pos = a.pos;
        dim_size = a.dim_size;
        return *this;
    }

    data_type & operator * ()
    {
        CHECK_RUN_BOUNDS (pos)
        return (BID < _blk_sz >
                &)((*(array[(pos) % dim_size]))[(pos) /
                                                dim_size].
                   bid);
    }

    const data_type * operator -> () const
    {
        CHECK_RUN_BOUNDS (pos)
        return
               &((*(array[(pos) % dim_size])[(pos) / dim_size].bid));
    }


    data_type & operator [](__pos_type n) const
    {
        n += pos;
        CHECK_RUN_BOUNDS (n)
        return (BID < _blk_sz >
                &)((*(array[(n) % dim_size]))[(n) /
                                              dim_size].bid);
    }
};

BLOCK_ADAPTOR_OPERATORS(RunsToBIDArrayAdaptor)

template < unsigned _blk_sz, typename _run_type, class __pos_type = int_type >
struct RunsToBIDArrayAdaptor2
: public TwoToOneDimArrayAdaptorBase < _run_type *, BID < _blk_sz >, __pos_type >
{
    typedef RunsToBIDArrayAdaptor2 < _blk_sz, _run_type, __pos_type > _Self;
    typedef BID < _blk_sz > data_type;

    typedef TwoToOneDimArrayAdaptorBase < _run_type *, BID < _blk_sz >, __pos_type> ParentClass_;

    using ParentClass_::pos;
    using ParentClass_::array;

    enum
    { block_size = _blk_sz };

    __pos_type w, h, K;

    RunsToBIDArrayAdaptor2 (_run_type * *a, __pos_type p, int_type _w,
                            int_type _h) : TwoToOneDimArrayAdaptorBase <
                                                                        _run_type *, BID < _blk_sz >, __pos_type > (a, p), w (_w),
                                           h (_h), K (_w * _h)
    { };

    RunsToBIDArrayAdaptor2 (const _Self &
                            a) : TwoToOneDimArrayAdaptorBase < _run_type *,
                                                              BID < _blk_sz >, __pos_type > (a), w (a.w), h (a.h),
                                 K (a.K)
    { };

    const _Self & operator = (const _Self & a)
    {
        array = a.array;
        pos = a.pos;
        w = a.w;
        h = a.h;
        K = a.K;
        return *this;
    }

    data_type & operator * ()
    {
        register __pos_type i = pos - K;
        if (i < 0)
            return (BID < _blk_sz >
                    &)((*(array[(pos) % w]))[(pos) / w].bid);


        register __pos_type _w = w;
        _w--;
        return (BID < _blk_sz >
                &)((*(array[(i) % _w]))[h + (i / _w)].bid);
    }

    const data_type * operator -> () const
    {
        register __pos_type i = pos - K;
        if (i < 0)
            return &((*(array[(pos) % w])[(pos) / w].bid));


        register __pos_type _w = w;
        _w--;
        return &((*(array[(i) % _w])[h + (i / _w)].bid));
    }


    data_type & operator [](__pos_type n) const
    {
        n += pos;
        register __pos_type i = n - K;
        if (i < 0)
            return (BID < _blk_sz >
                    &)((*(array[(n) % w]))[(n) / w].bid);


        register __pos_type _w = w;
        _w--;
        return (BID < _blk_sz >
                &)((*(array[(i) % _w]))[h + (i / _w)].bid);
    }
};

BLOCK_ADAPTOR_OPERATORS (RunsToBIDArrayAdaptor2)


template <typename trigger_iterator_type, unsigned _BlkSz>
struct trigger_entry_iterator
{
    typedef trigger_entry_iterator<trigger_iterator_type, _BlkSz> _Self;

    typedef BID<_BlkSz> bid_type;
    trigger_iterator_type value;

    // STL typedefs
    typedef bid_type value_type;
    typedef std::random_access_iterator_tag iterator_category;
    typedef int_type difference_type;
    typedef value_type * pointer;
    typedef value_type & reference;

    enum { block_size = _BlkSz };

    trigger_entry_iterator(const _Self &a) : value(a.value) { };
    trigger_entry_iterator(trigger_iterator_type v) : value(v) { };

    bid_type & operator * ()
    {
        return value->bid;
    }
    bid_type * operator -> () const
    {
        return &(value->bid);
    }
    const bid_type & operator [] (int_type n) const
    {
        return (value + n)->bid;
    }
    bid_type & operator [] (int_type n)
    {
        return (value + n)->bid;
    }

    _Self & operator ++()
    {
        value++;
        return *this;
    }
    _Self operator ++(int)
    {
        _Self __tmp = *this;
        value++;
        return __tmp;
    }
    _Self & operator --()
    {
        value--;
        return *this;
    }
    _Self operator --(int)
    {
        _Self __tmp = *this;
        value--;
        return __tmp;
    }
    bool operator == (const _Self &a) const
    {
        return value == a.value;
    }
    bool operator != (const _Self &a) const
    {
        return value != a.value;
    }
    _Self operator += (int_type n)
    {
        value += n;
        return *this;
    }
    _Self operator -= (int_type n)
    {
        value -= n;
        return *this;
    }
    int_type operator - (const _Self & a) const
    {
        return value - a.value;
    }
    int_type operator + (const _Self & a) const
    {
        return value + a.value;
    }
};



__STXXL_END_NAMESPACE

#endif
