#ifndef SWITCH_HEADER
#define SWITCH_HEADER

/***************************************************************************
 *            switch.h
 *	kind of semaphore
 *  Sat Aug 24 23:53:59 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "utils.h"
#include <pthread.h>

namespace stxxl
{

	//#define onoff_switch Switch

	class onoff_switch
	{
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		bool _on;
    onoff_switch(const onoff_switch & obj);
    onoff_switch & operator = (const onoff_switch & obj);
	public:
		onoff_switch (bool flag = false):_on (flag)
		{
			stxxl_nassert (pthread_mutex_init (&mutex, NULL));
			stxxl_nassert (pthread_cond_init (&cond, NULL));
		};
		 ~onoff_switch ()
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
		void on ()
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			_on = true;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_signal (&cond));
		};
		void off ()
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			_on = false;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_signal (&cond));
		};
		void wait_for_on ()
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			if (!_on)
				stxxl_nassert (pthread_cond_wait
					       (&cond, &mutex));
			stxxl_nassert (pthread_mutex_unlock (&mutex));
		};
		void wait_for_off ()
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			if (_on)
				stxxl_nassert (pthread_cond_wait
					       (&cond, &mutex));
			stxxl_nassert (pthread_mutex_unlock (&mutex));
		};
		bool is_on ()
		{
			bool res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _on;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			return res;
		};
	};
}

#endif
