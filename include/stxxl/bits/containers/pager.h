#ifndef PAGER_HEADER
#define PAGER_HEADER

/***************************************************************************
 *            pager.h
 *
 *  Sun Oct  6 14:49:19 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include <stxxl/bits/noncopyable.h>
#include "stxxl/bits/common/rand.h"
#include "stxxl/bits/common/simple_vector.h"
#include <stxxl/bits/compat_auto_ptr.h>

#include <list>


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

    compat_auto_ptr<list_type>::result history;
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
        // workaround for buggy GCC 3.4 STL
        //std::swap(history,obj.history);
        compat_auto_ptr<list_type>::result tmp = obj.history;
        obj.history = history;
        history = tmp;
        std::swap(history_entry, obj.history_entry);
    }
};

//! \}

__STXXL_END_NAMESPACE

namespace std
{
    template <unsigned npages_>
    void swap(stxxl::lru_pager < npages_ > & a,
              stxxl::lru_pager<npages_> & b)
    {
        a.swap(b);
    }
}

#endif
