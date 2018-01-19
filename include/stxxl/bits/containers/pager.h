/***************************************************************************
 *  include/stxxl/bits/containers/pager.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002, 2003, 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_PAGER_HEADER
#define STXXL_CONTAINERS_PAGER_HEADER

#include <algorithm>
#include <cassert>
#include <list>
#include <utility>

#include <tlx/simple_vector.hpp>
#include <tlx/unused.hpp>

#include <stxxl/bits/common/rand.h>

namespace stxxl {

//! \addtogroup stlcont_vector
//! \{

//! Pager with \b random replacement strategy
template <unsigned npages_>
class random_pager
{
    using size_type = size_t;

    size_type num_pages;
    random_number<random_uniform_fast> rnd;

public:
    static constexpr unsigned default_npages = npages_;

    explicit random_pager(size_type num_pages = default_npages)
        : num_pages(num_pages) { }

    size_type kick()
    {
        return rnd(size());
    }

    void hit(size_type ipage) const
    {
        assert(ipage < size());
        tlx::unused(ipage);
    }

    size_type size() const
    {
        return num_pages;
    }
};

//! Pager with \b LRU replacement strategy
template <unsigned npages_ = 0>
class lru_pager
{
    using size_type = size_t;
    using list_type = std::list<size_type>;

    list_type history;
    tlx::simple_vector<list_type::iterator> history_entry;

public:
    static constexpr unsigned default_npages = npages_;

    explicit lru_pager(size_type num_pages = default_npages)
        : history_entry(num_pages)
    {
        for (size_type i = 0; i < size(); ++i)
            history_entry[i] = history.insert(history.end(), i);
    }

    //! non-copyable: delete copy-constructor
    lru_pager(const lru_pager&) = delete;
    //! non-copyable: delete assignment operator
    lru_pager& operator = (const lru_pager&) = delete;

    size_type kick()
    {
        return history.back();
    }

    void hit(size_type ipage)
    {
        assert(ipage < size());
        history.splice(history.begin(), history, history_entry[ipage]);
    }

    void swap(lru_pager& obj)
    {
        history.swap(obj.history);
        history_entry.swap(obj.history_entry);
    }

    size_type size() const
    {
        return history_entry.size();
    }
};

//! \}

} // namespace stxxl

namespace std {

template <unsigned npages_>
void swap(stxxl::lru_pager<npages_>& a,
          stxxl::lru_pager<npages_>& b)
{
    a.swap(b);
}

} // namespace std

#endif // !STXXL_CONTAINERS_PAGER_HEADER
// vim: et:ts=4:sw=4
