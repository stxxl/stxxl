/***************************************************************************
 *  include/stxxl/bits/containers/pager.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2003, 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
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
    typedef unsigned_type size_type;
    size_type num_pages;
    random_number<random_uniform_fast> rnd;

public:
    enum { n_pages = npages_ };
    random_pager(size_type num_pages = n_pages) : num_pages(num_pages) { }
    int_type kick()
    {
        return rnd(num_pages);
    }
    void hit(int_type ipage)
    {
        assert(ipage < int_type(num_pages));
        assert(ipage >= 0);
    }
    size_type size () const
    {
        return num_pages;
    }
};

//! \brief Pager with \b LRU replacement strategy
template <unsigned npages_ = 0>
class lru_pager : private noncopyable
{
    typedef unsigned_type size_type;
    typedef std::list<size_type> list_type;

    list_type history;
    simple_vector<list_type::iterator> history_entry;

public:
    enum { n_pages = npages_ };

    lru_pager(size_type num_pages = n_pages) : history_entry(num_pages)
    {
        for (size_type i = 0; i < history_entry.size(); ++i)
            history_entry[i] = history.insert(history.end(), i);
    }

    size_type kick()
    {
        return history.back();
    }

    void hit(size_type ipage)
    {
        assert(ipage < history_entry.size());
        history.splice(history.begin(), history, history_entry[ipage]);
    }

    void swap(lru_pager & obj)
    {
        history.swap(obj.history);
        history_entry.swap(obj.history_entry);
    }

    size_type size () const
    {
        return history_entry.size();
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
// vim: et:ts=4:sw=4
