#ifndef RUN_CURSOR_HEADER
#define RUN_CURSOR_HEADER

/***************************************************************************
 *            run_cursor.h
 *
 *  Mon Jan 20 10:38:01 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "../common/utils.h"

__STXXL_BEGIN_NAMESPACE

template <typename block_type>
struct run_cursor
{
	unsigned int pos;
	block_type *buffer;

	inline const typename block_type::type & current () const
	{
		return (*buffer)[pos];
	}
	inline void operator ++ (int)
	{
		pos++;
	}
};


struct have_prefetcher
{
	static void * untyped_prefetcher;
};

template <typename block_type,
					typename prefetcher_type_>
struct run_cursor2:public run_cursor<block_type>,public have_prefetcher
{
	typedef prefetcher_type_ prefetcher_type;
	typedef run_cursor2<block_type,prefetcher_type> _Self;
	typedef typename block_type::value_type value_type;
											
	static prefetcher_type *& prefetcher() // sorry, a hack
	{
		return (prefetcher_type * &) untyped_prefetcher;
	};
	
	run_cursor2 () { };
	
	inline bool empty () const
	{
		return (pos >= block_type::size);
	};
	inline void operator ++ (int);
	inline void make_inf ()
	{
		pos = block_type::size;
	};
};

void * have_prefetcher::untyped_prefetcher = NULL;

template <typename block_type,
					typename prefetcher_type>
void
run_cursor2<block_type,prefetcher_type>::operator ++ (int)
{
	pos++;
	if (UNLIKELY(pos >= block_type::size))
	{
		if( prefetcher()->block_consumed(buffer) ) pos = 0;
	}
};



template <typename block_type>
struct run_cursor_cmp
{
	typedef run_cursor<block_type> cursor_type;
	
	inline bool operator  () (const cursor_type & a, const cursor_type & b)	// greater or equal
	{
		return !((*a.buffer)[a.pos] < (*b.buffer)[b.pos]);
	};
};

__STXXL_END_NAMESPACE


#endif
