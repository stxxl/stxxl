#ifndef RWLOCK_HEADER
#define RWLOCK_HEADER

/***************************************************************************
 *            rwlock.h
 *
 *  Sat Aug 24 23:53:44 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "utils.h"

#if 0 // not used

#include <pthread.h>


namespace stxxl
{

	class RWLock
	{
		pthread_rwlock_t rwlock;
	      public:

		  RWLock ()
		{
			stxxl_nassert (pthread_rwlock_init (&rwlock, NULL));
		};
		 ~RWLock ()
		{
			stxxl_nassert (pthread_rwlock_destroy (&rwlock));
		};
		void rdlock ()
		{
			stxxl_nassert (pthread_rwlock_rdlock (&rwlock));
		};
		void wrlock ()
		{
			stxxl_nassert (pthread_rwlock_rdlock (&wrlock));
		};
		void unlock ()
		{
			stxxl_nassert (pthread_rwlock_unlock (&mutex));
		};
	};

}

#endif

#endif
