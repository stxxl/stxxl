/***************************************************************************
 *  include/stxxl/bits/containers/btree/btree_pager.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE__BTREE_PAGER_H
#define STXXL_CONTAINERS_BTREE__BTREE_PAGER_H

#include <memory>
#include <list>

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/compat_unique_ptr.h>


__STXXL_BEGIN_NAMESPACE

namespace btree
{
    class lru_pager : private noncopyable
    {
        unsigned_type npages_;
        typedef std::list<int_type> list_type;

        compat_unique_ptr<list_type>::result history;
        std::vector<list_type::iterator> history_entry;

    public:
        lru_pager() : npages_(0)
        { }

        lru_pager(unsigned_type npages) :
            npages_(npages),
            history(new list_type),
            history_entry(npages_)
        {
            for (unsigned_type i = 0; i < npages_; i++)
            {
                history_entry[i] = history->insert(history->end(), static_cast<int_type>(i));
            }
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
            std::swap(npages_, obj.npages_);
            std::swap(history, obj.history);
            std::swap(history_entry, obj.history_entry);
        }
    };
}

__STXXL_END_NAMESPACE


namespace std
{
    inline void swap(stxxl::btree::lru_pager & a,
                     stxxl::btree::lru_pager & b)
    {
        a.swap(b);
    }
}

#endif /* STXXL_CONTAINERS_BTREE__BTREE_PAGER_H */
