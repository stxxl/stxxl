/***************************************************************************
                          btree_queue.h  -  description
                             -------------------
    begin                : Mit Jun 16 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file btree_queue.h
//! \brief Implemets the btree query queue \c stxxl::map_internal::btree_queue for the B-Tree implemantation for \c stxxl::map container.

#ifndef BTREE_QUEUE_H
#define BTREE_QUEUE_H

#include "smart_ptr.h"
#include "btree_request.h"


__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	template<typename Btree> class btree_queue;

	//! A queue for requests for btree of \c stxxl::map container.
	template<typename Btree>
	class btree_queue
	{
	public:

		btree_queue() : sem(0), _is_empty(1)
		{
			stxxl_nassert( pthread_create( &thread, NULL, (thread_function_t) worker, static_cast<void *>( this ) ) );
		}

		//! destructor waits for requests
		~btree_queue()
		{
			// _is_empty.wait_for( 1 );

			smart_ptr<btree_request<Btree> > req;
			_mutex.lock ();
			while( !worker_queue.empty () )
			{
				req = worker_queue.front ();
				_mutex.unlock ();

				req->wait();

				_mutex.lock ();
			}
			_mutex.unlock ();
			stxxl_nassert ( pthread_cancel ( thread ) );
		}

		//! add a request into queue
		void add_request ( btree_request<Btree>* p_req )
		{
			_is_empty.set_to( 0 );
			_mutex.lock ();
			worker_queue.push ( smart_ptr<btree_request<Btree> >( p_req ) );
			_mutex.unlock ();
			sem++;
		}

	private:

		pthread_t thread;
		semaphore sem;
		mutex _mutex;
		state _is_empty;

		//! works the queries
		/*!
		We serve a request so long as the nodes it needs are in the locale cache.
		( serve returns btree_request_base::OP )
		*/
		std::queue< smart_ptr<btree_request<Btree> > > worker_queue;
		static void *btree_queue::worker (void *arg)
		{
			//sp_req;
			btree_queue *pthis = static_cast<btree_queue*>(arg);
			stxxl_nassert (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
			stxxl_nassert (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));

			int empty;
			smart_ptr<btree_request<Btree> > sp_req;
			for (;;)
			{
				// Btree::MrProper();

				pthis->sem--;
				pthis->_mutex.lock ();
				sp_req = pthis->worker_queue.front ();
				pthis->worker_queue.pop ();
				pthis->_mutex.unlock ();


				while( sp_req->serve() == btree_request_base::OP )
				{
				};

				pthis->_mutex.lock ();
				empty = pthis->worker_queue.size();
				pthis->_mutex.unlock ();

				if ( empty ) pthis->_is_empty.set_to( 1 );
			}
			return NULL;
		}
	};

	//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE

#endif
