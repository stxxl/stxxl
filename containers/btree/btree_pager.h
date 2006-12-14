/***************************************************************************
 *            btree_pager.h
 *
 *  Wed Feb 15 11:55:33 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _BTREE_PAGER_H
#define _BTREE_PAGER_H

#include <stxxl>
#include <list>

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	class lru_pager
	{
        unsigned npages_;
        typedef std::list<int> list_type;

		std::auto_ptr<list_type> history;
		std::vector<list_type::iterator> history_entry;

	
        lru_pager(const lru_pager & obj);
        lru_pager & operator = (const lru_pager & obj);
	public:
			
        lru_pager(): npages_(0),history(NULL)
		{
		}

        lru_pager(unsigned npages): 
			npages_(npages),
			history(new list_type),
			history_entry(npages_)
        {
                for(unsigned i=0;i<npages_;i++)
				{
                        history_entry[i] = history->insert(history->end(),static_cast<int>(i));
				}
        }
        ~lru_pager() {}
        int kick()
        {
                return history->back();
        }
        void hit(int ipage)
        {
                assert(ipage < int(npages_));
                assert(ipage >= 0);
                history->splice(history->begin(),*history,history_entry[ipage]);
        }
        void swap(lru_pager & obj)
        {
		std::swap(npages_,obj.npages_);
		// workaround for buggy GCC 3.4 STL
                //std::swap(history,obj.history);
		std::auto_ptr<list_type> tmp = obj.history;
		obj.history = history;
		history = tmp;
                std::swap(history_entry,obj.history_entry);
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


#endif /* _BTREE_PAGER_H */
