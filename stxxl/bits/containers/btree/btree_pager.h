/***************************************************************************
 *            btree_pager.h
 *
 *  Wed Feb 15 11:55:33 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE__BTREE_PAGER_H
#define STXXL_CONTAINERS_BTREE__BTREE_PAGER_H

#include <memory>
#include <list>

#include "stxxl/bits/common/utils.h"


__STXXL_BEGIN_NAMESPACE

namespace btree
{
    class lru_pager
    {
        unsigned_type npages_;
        typedef std::list<int_type> list_type;

        std::auto_ptr<list_type> history;
        std::vector<list_type::iterator> history_entry;


        lru_pager(const lru_pager & obj);
        lru_pager & operator = (const lru_pager & obj);
    public:

        lru_pager() : npages_(0), history(NULL)
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
            // workaround for buggy GCC 3.4 STL
            //std::swap(history,obj.history);
            std::auto_ptr<list_type> tmp = obj.history;
            obj.history = history;
            history = tmp;
            std::swap(history_entry, obj.history_entry);
        }
    };
}

__STXXL_END_NAMESPACE


namespace std
{
    inline void swap(      stxxl::btree::lru_pager & a,
                           stxxl::btree::lru_pager & b)
    {
        a.swap(b);
    }
}


#endif /* STXXL_CONTAINERS_BTREE__BTREE_PAGER_H */
