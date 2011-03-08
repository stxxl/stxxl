/***************************************************************************
 *  include/stxxl/bits/containers/pager.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PAGER_HEADER
#define STXXL_PAGER_HEADER

#include <list>
#include <cassert>

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/rand.h>
#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/compat_unique_ptr.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//! \{

enum pager_type
{
    random,
    lru
};

//! \brief Pager with \b random replacement strategy
template <unsigned npages_>
class random_pager
{
    random_number<random_uniform_fast> rnd;

public:
    enum { n_pages = npages_ };
    random_pager() { }
    ~random_pager() { }
    int_type kick()
    {
        return rnd(npages_);
    }
    void hit(int_type ipage)
    {
        assert(ipage < int_type(npages_));
        assert(ipage >= 0);
    }
};

//! \brief Pager with \b LRU replacement strategy
template <unsigned npages_>
class lru_pager : private noncopyable
{
    typedef std::list<int_type> list_type;

    compat_unique_ptr<list_type>::result history;
    simple_vector<list_type::iterator> history_entry;

public:
    enum { n_pages = npages_ };

    lru_pager() : history(new list_type), history_entry(npages_)
    {
        for (unsigned_type i = 0; i < npages_; i++)
            history_entry[i] = history->insert(history->end(), static_cast<int_type>(i));
    }
    ~lru_pager() { }
    int_type kick()
    {
        return history->back();
    }
    void hit(int_type ipage)
    {
        assert(ipage < int_type(npages_));
        assert(ipage >= 0);
        history->splice(history->begin(), *history, history_entry[ipage]);
    }
    void swap(lru_pager & obj)
    {
        std::swap(history, obj.history);
        std::swap(history_entry, obj.history_entry);
    }
};

//! \}

__STXXL_END_NAMESPACE

namespace std
{
    template <unsigned npages_>
    void swap(stxxl::lru_pager<npages_> & a,
              stxxl::lru_pager<npages_> & b)
    {
        a.swap(b);
    }
}

#endif // !STXXL_PAGER_HEADER
