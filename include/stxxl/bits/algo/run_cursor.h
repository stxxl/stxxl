/***************************************************************************
 *  include/stxxl/bits/algo/run_cursor.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_RUN_CURSOR_HEADER
#define STXXL_RUN_CURSOR_HEADER

#include <stxxl/bits/common/utils.h>


__STXXL_BEGIN_NAMESPACE

template <typename block_type>
struct run_cursor
{
    unsigned pos;
    block_type * buffer;

    run_cursor() : pos(0), buffer(NULL) { }

    inline const typename block_type::type & current() const
    {
        return (*buffer)[pos];
    }
    inline void operator ++ (int)
    {
        pos++;
    }
};

#ifdef STXXL_SORT_SINGLE_PREFETCHER

template <typename must_be_void = void>
struct have_prefetcher
{
    static void * untyped_prefetcher;
};

#endif

template <typename block_type,
          typename prefetcher_type_>
struct run_cursor2 : public run_cursor<block_type>
#ifdef STXXL_SORT_SINGLE_PREFETCHER
                     , public have_prefetcher<>
#endif
{
    typedef prefetcher_type_ prefetcher_type;
    typedef run_cursor2<block_type, prefetcher_type> _Self;
    typedef typename block_type::value_type value_type;


    using run_cursor<block_type>::pos;
    using run_cursor<block_type>::buffer;

#ifdef STXXL_SORT_SINGLE_PREFETCHER
    static prefetcher_type * const prefetcher()    // sorry, a hack
    {
        return reinterpret_cast<prefetcher_type *>(untyped_prefetcher);
    }
    static void set_prefetcher(prefetcher_type * pfptr)
    {
        untyped_prefetcher = pfptr;
    }
    run_cursor2() { }
#else
    prefetcher_type * prefetcher_;
    prefetcher_type * & prefetcher() // sorry, a hack
    {
        return prefetcher_;
    }

    run_cursor2() { }
    run_cursor2(prefetcher_type * p) : prefetcher_(p) { }
#endif

    inline bool empty() const
    {
        return (pos >= block_type::size);
    }
    inline void operator ++ (int);
    inline void make_inf()
    {
        pos = block_type::size;
    }
};

#ifdef STXXL_SORT_SINGLE_PREFETCHER
template <typename must_be_void>
void * have_prefetcher<must_be_void>::untyped_prefetcher = NULL;
#endif

template <typename block_type,
          typename prefetcher_type>
void run_cursor2<block_type, prefetcher_type>::operator ++ (int)
{
    assert(!empty());
    pos++;
    if (UNLIKELY(pos >= block_type::size))
    {
        if (prefetcher()->block_consumed(buffer))
            pos = 0;
    }
}


template <typename block_type>
struct run_cursor_cmp
{
    typedef run_cursor<block_type> cursor_type;
    /*
       inline bool operator  () (const cursor_type & a, const cursor_type & b)	// greater or equal
       {
            return !((*a.buffer)[a.pos] < (*b.buffer)[b.pos]);
       }*/
};

__STXXL_END_NAMESPACE


#endif // !STXXL_RUN_CURSOR_HEADER
