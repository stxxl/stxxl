/***************************************************************************
                          bt_counter.h  -  description
                             -------------------
    begin                : Die Okt 12 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file containers//map//bt_counter.h
//! \brief Implemets a helpfully \c stxxl::map_internal::bt_counter counter for \c stxxl::map implementation.

#ifndef ___BT_COUNTER__H___
#define ___BT_COUNTER__H___

namespace stxxl
{
namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	//! \brief A helpfull class for asyncron counting.
	/*! All functions are thread secure.
	We can use this class for counting and with the synchronisation,
	in order to wait for a certain value.
	*/
	class bt_counter
	{

			
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		unsigned long long _count;

	public:

		
		bt_counter (int c = 0):_count (c) //! Contructor for bt_counter.
		{
			stxxl_nassert (pthread_mutex_init (&mutex, NULL));
			stxxl_nassert (pthread_cond_init (&cond, NULL));
		};

		//! \brief Destructor for bt_counter.
		~bt_counter ()
		{
			int res = pthread_mutex_trylock (&mutex);

			if (res == 0 || res == EBUSY)
				stxxl_nassert (pthread_mutex_unlock(&mutex))
			else
				stxxl_function_error
				stxxl_nassert (pthread_mutex_destroy(&mutex));
			  stxxl_nassert (pthread_cond_destroy (&cond));
		};

		//! \brief Set the counter to a new value.
		//! \param new_count the new value for counter.
		void set_to (unsigned long long new_count)
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			_count = new_count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
		};

		//! \brief Wait until the counter reach a value.
		//! \param needed_count the new value we are waiting for.
		void wait_for (unsigned long long needed_count)
		{
			stxxl_nassert (pthread_mutex_lock (&mutex));
			while (needed_count != _count)
				stxxl_nassert (pthread_cond_wait(&cond, &mutex));
			stxxl_nassert (pthread_mutex_unlock (&mutex));
		};

		//! \brief Increase the counter.
		unsigned long long operator++()
		{
			int res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = ++_count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			return res;
		}

		//! \brief Increase the counter.
		unsigned long long operator++(int)
		{
			int res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _count++;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			return res;
		}

		//! \brief Decrease the counter.
		unsigned long long operator--()
		{
			int res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = --_count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			return res;
		}

		//! \brief Decrease the counter.
		unsigned long long operator--(int)
		{
			int res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _count--;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			return res;
		}

		//! \brief Access to the value of couter.
		unsigned long long operator () ()
		{
			int res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			return res;
		};
	};

	//! \}
} // namespace map_internal

} // namespace stxxl

#endif  // ___BT_COUNTER__H___


