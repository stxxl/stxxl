#ifndef STATE_HEADER
#define STATE_HEADER
/***************************************************************************
 *            state.h
 *
 *  Sat Aug 24 23:53:53 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "utils.h"
#include <pthread.h>

namespace stxxl
{

	class state
	{
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		int _state;
	      public:
		  state (int s = 0):_state (s)
		{
			stxxl_nassert (pthread_mutex_init (&mutex, NULL));
			stxxl_nassert (pthread_cond_init (&cond, NULL));
		};
		 ~state ()
		{
			int res = pthread_mutex_trylock (&mutex);

			if (res == 0 || res == EBUSY)
				  stxxl_nassert (pthread_mutex_unlock
						 (&mutex))
				else
				  stxxl_function_error
					stxxl_nassert (pthread_mutex_destroy
						       (&mutex));

			  stxxl_nassert (pthread_cond_destroy (&cond));
		};
		void set_to (int new_state)
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			_state = new_state;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
		};
		void wait_for (int needed_state)
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			while (needed_state != _state)
				stxxl_nassert (pthread_cond_wait
					       (&cond, &mutex));
			stxxl_nassert (pthread_mutex_unlock (&mutex));
		};
		int operator () ()
		{
			int res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _state;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			return res;
		};
	};


}

#endif
