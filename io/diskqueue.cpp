#ifndef DISKQUEUE_HEADER
#define DISKQUEUE_HEADER

/***************************************************************************
 *            diskqueue.cpp
 *
 *  Sat Aug 24 23:52:06 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "iobase.h"
#include "../common/gprof.h"

#ifdef STXXL_THREAD_PROFILING
#define pthread_create gprof_pthread_create
#endif

namespace stxxl
{

	disk_queue::disk_queue (int n):sem (0), _priority_op (WRITE)	//  n is ignored
	{
		//  cout << "disk_queue created." << endl;
#ifdef STXXL_IO_STATS
		iostats = stats::get_instance ();
#endif
		stxxl_nassert(pthread_create(&thread, NULL, (thread_function_t) worker, static_cast<void *>(this)));
	}

	void disk_queue::add_readreq (request_ptr & req)
	{
		read_mutex.lock ();
		read_queue.push (req);
		read_mutex.unlock ();
		sem++;
	}

	void disk_queue::add_writereq (request_ptr & req)
	{

		//  cout << req << " submitted" << endl;
		write_mutex.lock ();
		write_queue.push (req);
		write_mutex.unlock ();
		sem++;
		//  cout << req << " added"<< endl;
	}

	disk_queue::~disk_queue ()
	{
		stxxl_nassert (pthread_cancel (thread));
	}

	void *disk_queue::worker (void *arg)
	{
		disk_queue *pthis = static_cast<disk_queue*>(arg);
		request_ptr req;

		stxxl_nassert (pthread_setcancelstate
			       (PTHREAD_CANCEL_ENABLE, NULL));
		stxxl_nassert (pthread_setcanceltype
			       (PTHREAD_CANCEL_DEFERRED, NULL));
		// Allow cancellation in semaphore operator-- call 

		bool write_phase = true;
		for (;;)
		{
			pthis->sem--;

			if (write_phase)
			{
				pthis->write_mutex.lock ();
				if (!pthis->write_queue.empty ())
				{
					req = pthis->write_queue.front ();
					pthis->write_queue.pop ();

					pthis->write_mutex.unlock ();

					//assert(req->nref() > 1);
					req->serve ();

				}
				else
				{
					pthis->write_mutex.unlock ();

					pthis->sem++;

					if (pthis->_priority_op == WRITE)
						write_phase = false;

				}

				if (pthis->_priority_op == NONE
				    || pthis->_priority_op == READ)
					write_phase = false;

			}
			else
			{

				pthis->read_mutex.lock ();
				if (!pthis->read_queue.empty ())
				{
					req = pthis->read_queue.front ();
					pthis->read_queue.pop ();
					pthis->read_mutex.unlock ();

					STXXL_VERBOSE2( "queue: before serve request has "<< req->nref()<< " references ")
					//assert(req->nref() > 1);
					req->serve ();
					STXXL_VERBOSE2( "queue: after serve request has "<< req->nref()<< " references ")
					
				}
				else
				{
					pthis->read_mutex.unlock ();

					pthis->sem++;

					if (pthis->_priority_op == READ)
						write_phase = true;
				}

				if (pthis->_priority_op == NONE
				    || pthis->_priority_op == WRITE)
					write_phase = true;
			}
		}

		return NULL;
	}



};

#endif
