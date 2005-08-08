/***************************************************************************
                          map.h  -  description
                             -------------------
    begin                : Fre Feb 11 2005
    copyright            : (C) 2005 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/
//! \file map.h
//! \brief This file contains a halpfully macros.

#include <mng/mng.h>
#ifndef BOOST_MSVC
#include <pthread.h>
#endif
#include <queue>
#include <common/semaphore.h>
#include <common/mutex.h>
#include "smart_ptr.h"

/*STXXL_ASSERT(expr) \
	{\
		int ass_res=expr; \
		if(!ass_res) { \
		 std::cerr << "Error in function: " << \
		 __PRETTY_FUNCTION__ << " ";  \
		 stxxl_passert(__STXXL_STRING(expr)); }}*/

#ifndef ___MAP_H___
#define ___MAP_H___



__STXXL_BEGIN_NAMESPACE
	template < unsigned SIZE_ >
	bool operator < (const BID<SIZE_> & a, const BID<SIZE_> & b)
	{
		return (long) a.storage < (long) b.storage || (long) a.storage == (long) b.storage && a.offset < b.offset ;
	};
__STXXL_END_NAMESPACE

	
__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

//! \brief Internal implementation of \c stxxl::map container.
namespace map_internal
{

// ************************************************************************
// For speed up the test we can create smaller nodes.
// For example we can compile
// -DSTXXL_MAP_MAX_COUNT=6 means the max_size of nodes is 6
// This allows us to create trees,
// which has more levels but contains less entries.
// ************************************************************************
#ifdef STXXL_MAP_MAX_COUNT

#define ___STXXL_MAP_MAX_COUNT___ STXXL_MAP_MAX_COUNT

#else

#define ___STXXL_MAP_MAX_COUNT___ ( SIZE_ - sizeof(int) - 2* sizeof( BID<SIZE_> )  )  / sizeof (value_type)

#endif

}

//! \}

__STXXL_END_NAMESPACE

#ifdef STXXL_DEBUG_ON
#define STXXL_ASSERT(expr) assert( expr );
#else
#define STXXL_ASSERT(expr)
#endif

#define _SIZE_TYPE 									stxxl::uint64

#endif // ___MAP_H___
