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
		#ifdef STXXL_BOOST_THREADS
		boost::mutex mutex;
		boost::condition cond;
		#else
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		#endif
		stxxl::uint64 _count;

	public:

		
		bt_counter (int c = 0):_count (c) //! Contructor for bt_counter.
		{
			#ifndef STXXL_BOOST_THREADS
			stxxl_nassert (pthread_mutex_init (&mutex, NULL));
			stxxl_nassert (pthread_cond_init (&cond, NULL));
			#endif
		};

		//! \brief Destructor for bt_counter.
		~bt_counter ()
		{
			#ifndef STXXL_BOOST_THREADS
			int res = pthread_mutex_trylock (&mutex);

			if (res == 0 || res == EBUSY)
				stxxl_nassert (pthread_mutex_unlock(&mutex))
			else
				stxxl_function_error
				stxxl_nassert (pthread_mutex_destroy(&mutex));
			  stxxl_nassert (pthread_cond_destroy (&cond));
			#endif
		};

		//! \brief Set the counter to a new value.
		//! \param new_count the new value for counter.
		void set_to (stxxl::uint64 new_count)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(mutex);
			_count = new_count;
			Lock.unlock();
			cond.notify_all();
			#else
			stxxl_nassert (pthread_mutex_lock (&mutex));
			_count = new_count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			#endif
		};

		//! \brief Wait until the counter reach a value.
		//! \param needed_count the new value we are waiting for.
		void wait_for (stxxl::uint64 needed_count)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(mutex);
			while (needed_count != _count)
				cond.wait(Lock);
			#else
			stxxl_nassert (pthread_mutex_lock (&mutex));
			while (needed_count != _count)
				stxxl_nassert (pthread_cond_wait(&cond, &mutex));
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			#endif
		};

		//! \brief Increase the counter.
		stxxl::uint64 operator++()
		{
			int res;
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(mutex);
			res = ++_count;
			Lock.unlock();
			cond.notify_all();
			#else
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = ++_count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			#endif
			return res;
		}

		//! \brief Increase the counter.
		stxxl::uint64 operator++(int)
		{
			int res;
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(mutex);
			res = _count++;
			Lock.unlock();
			cond.notify_all();
			#else
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _count++;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			#endif
			return res;
		}

		//! \brief Decrease the counter.
		stxxl::uint64 operator--()
		{
			int res;
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(mutex);
			res = --_count;
			Lock.unlock();
			cond.notify_all();
			#else
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = ++_count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			#endif
			return res;
		}

		//! \brief Decrease the counter.
		stxxl::uint64 operator--(int)
		{
			int res;
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(mutex);
			res = _count--;
			Lock.unlock();
			cond.notify_all();
			#else
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _count++;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			stxxl_nassert (pthread_cond_broadcast (&cond));
			#endif
			return res;
		}

		//! \brief Access to the value of couter.
		stxxl::uint64 operator () ()
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(mutex);
			return _count;
			#else
			int res;
			stxxl_nassert (pthread_mutex_lock (&mutex));
			res = _count;
			stxxl_nassert (pthread_mutex_unlock (&mutex));
			return res;
			#endif
		};
	};

	//! \}
} // namespace map_internal

} // namespace stxxl

#endif  // ___BT_COUNTER__H___
