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

//! \brief Pager with \b random replacement strategy
template <unsigned npages_>
class random_pager
{
	random_number<random_uniform_fast> rnd;
public:
	enum { n_pages = npages_ };
	random_pager() {};
	~random_pager() {};
	int kick()
	{
		return rnd(npages_);
	};
	void hit(int ipage) {};
};

//! \brief Pager with \b LRU replacement strategy
template <unsigned npages_>
class lru_pager
{
	typedef std::list<int> list_type;
	
	list_type history;
	simple_vector<list_type::iterator> history_entry;
public:
	enum { n_pages = npages_ };
	
	lru_pager(): history_entry(npages_)
	{
		for(unsigned i=0;i<npages_;i++)
			history_entry[i] = history.insert(history.end(),static_cast<int>(i));
	};
	~lru_pager() {};
	int kick()
	{
		return history.back();
	};
	void hit(int ipage)
	{
		history.splice(history.begin(),history,history_entry[ipage]);
	};
};

__STXXL_END_NAMESPACE

#endif

