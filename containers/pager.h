#ifndef PAGER_HEADER
#define PAGER_HEADER

/***************************************************************************
 *            pager.h
 *
 *  Sun Oct  6 14:49:19 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
 
#include "../common/utils.h"
#include "../common/rand.h"
#include "../common/simple_vector.h"
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
	random_pager() {};
	~random_pager() {};
	int_type kick()
	{
		return rnd(npages_);
	};
	void hit(int_type ipage)
	{
		assert(ipage < int_type(npages_));
		assert(ipage >= 0);
	};
};

//! \brief Pager with \b LRU replacement strategy
template <unsigned npages_>
class lru_pager
{
	typedef std::list<int_type> list_type;
	
	std::auto_ptr<list_type> history;
	simple_vector<list_type::iterator> history_entry;
	
private:
	lru_pager(const lru_pager &);
	lru_pager & operator = (const lru_pager &); // forbidden
public:
	enum { n_pages = npages_ };
	
	lru_pager(): history(new list_type),history_entry(npages_)
	{
		for(unsigned_type i=0;i<npages_;i++)
			history_entry[i] = history->insert(history->end(),static_cast<int_type>(i));
	}
	~lru_pager() {}
	int_type kick()
	{
		return history->back();
	}
	void hit(int_type ipage)
	{
		assert(ipage < int_type(npages_));
		assert(ipage >= 0);
		history->splice(history->begin(),*history,history_entry[ipage]);
	}
	void swap(lru_pager & obj)
	{
		std::swap(history,obj.history);
		std::swap(history_entry,obj.history_entry);
	}
};

//! \}

__STXXL_END_NAMESPACE

namespace std
{
	template <unsigned npages_>
	void swap(	stxxl::lru_pager<npages_> & a,
				stxxl::lru_pager<npages_> & b)
	{
		a.swap(b);
	}
}

#endif
