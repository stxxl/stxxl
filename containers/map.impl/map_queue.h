/***************************************************************************
                          map_queue.h  -  description
                             -------------------
    begin                : Mit Jun 16 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

 //! \file map_queue.h
 //! \brief Implementing of quiries queue and locks for the \c stxxl::map container.

#ifndef MAP_QUEUE_H
#define MAP_QUEUE_H

#include "map_request.h"

__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	//! \brief Class coordinates the accesses to Mape.
	/*! This makes possible to be processed us the work with the map without consideration
	for the sequence of our requests.
	In order to simplify the implementation, with all write accesses conflicts are produced. */
	template<typename _Map>
	class map_lock : public map_lock_base
	{
		typedef map_lock _Self;

		typedef typename _Map::key_type            key_type;
		unsigned long    _object;
		int              _type;
		key_type         _lower;
		key_type         _upper;

		
public:

		//! \todo The lock strategy can be modified.
		//! Not all exclusive \b LOCKS cause wahers a conflict.
		//! And some of such to inquire can be implemented more paraller.
		enum
		{
			CLOSE_CLOSE = 0,
			CLOSE_OPEN = 1,
			OPEN_CLOSE = 2,
			OPEN_OPEN = 3,
		};

		//! a constructor
		map_lock( unsigned long object, int type, const key_type& lower, const key_type& upper, int l ) :
				map_lock_base(l), _object(object), _type(type), _lower( lower ), _upper( upper ) {}

		//! a prity print function
		std::ostream& operator <<( std::ostream & str )
		{
			char sta[5] ;
			switch( _state() )
			{
				case NO: sta[0] = 'N'; sta[1] = 'O'; sta[2] = '\0'; break;
				case S:  sta[0] = 'S'; sta[1] = '\0'; break;
				case X:  sta[0] = 'X'; sta[1] = '\0'; break;
			}

			switch( _type )
			{
				case CLOSE_CLOSE: str << "LOCK " << (void*) _object << ":" << this << " (" << sta << ")\t[" << _lower << ":" << _upper << "]" ; break;
				case CLOSE_OPEN:  str << "LOCK " << (void*) _object << ":" << this << " (" << sta << ")\t[" << _lower << ":OPEN]" ; break;
				case OPEN_CLOSE:  str << "LOCK " << (void*) _object << ":" << this << " (" << sta << ")\t[OPEN:" << _upper << "]" ; break;
				case OPEN_OPEN:   str << "LOCK " << (void*) _object << ":" << this << " (" << sta << ")\t[OPEN:OPEN]" ; break;
			}
			return str;
		}

		//! Function returns \b true to whom with the current \b LOCK to be deleted can.
		bool lock( _Self& x )
		{
			switch( _state() )
			{
				case X:
					_state.wait_for( NO );
					return true;

				case S:
					if( x._state() == X)
					{
						_state.wait_for( NO );
						return true;
					}
					else
						return false;
				
				case NO: return true;
			}
			return false;
		}
	};
	
	template<typename _Map>
	std::ostream& operator <<( std::ostream & str, map_lock<_Map>& x )
	{
		x.operator<<( str );
		return str;
	}


	//! \brief A queue for the quiries to the stxxl::map
	template<typename _Map>
	class map_queue
	{

	protected:

		typedef _Map 																map;
		typedef map_lock<_Map> 											lock_type;
		typedef std::vector<lock_type*> 						lock_table_type;
		typedef typename lock_table_type::iterator	lock_table_iterator;
		typedef typename map::btree_type 						btree;
		typedef typename map::key_type 							key_type;
		typedef typename _Map::node_manager_type   node_manager_type;
		typedef typename _Map::leaf_manager_type   leaf_manager_type;

	public: 

		map_queue() : sem (0)
		#ifdef STXXL_BOOST_THREADS
			,thread(boost::bind(worker,static_cast<void *>( this )))
		#endif
		{
			#ifndef STXXL_BOOST_THREADS
			stxxl_nassert( pthread_create(
				&thread,
				NULL,
				(thread_function_t) worker,
				static_cast<void *>( this ) ) );
			#endif
		}

		static char* GetTyp()
		{
			return _Map::GetTyp();
		}

		//! destructor, wich waits for requests
		~map_queue()
		{
			smart_ptr< map_request<map> > req;
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(this->worker_mutex);
			while( !this->worker_queue.empty () )
			{
				req = this->worker_queue.front ();
				Lock.unlock();
				req->wait();
				Lock.lock();
			}
			// TODO/possible BUG: MUST DESTROY THE THREAD HERE!!
			#else
			this->worker_mutex.lock ();
			while( !this->worker_queue.empty () )
			{
				req = this->worker_queue.front ();
				this->worker_mutex.unlock ();
				req->wait();
				this->worker_mutex.lock ();
			}
			this->worker_mutex.unlock ();
			stxxl_nassert ( pthread_cancel ( thread ) );
			#endif
		}

		//! append a request into queue.
		void add_request ( smart_ptr< map_request<map> > & p_req )
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(worker_mutex);
			worker_queue.push ( p_req );
			Lock.unlock();
			#else
			worker_mutex.lock ();
			worker_queue.push ( p_req );
			worker_mutex.unlock ();
			#endif
			this->sem++;
		}

	private:

		#ifdef STXXL_BOOST_THREADS
		boost::mutex worker_mutex;
		boost::mutex lock_mutex;
		#else
		mutex worker_mutex;
		mutex lock_mutex;
		#endif

		lock_table_type 																	lock_table;
		std::queue< smart_ptr < map_request<map> > >			worker_queue;
		semaphore sem;

		#ifdef STXXL_BOOST_THREADS
		boost::thread thread;
		#else
		pthread_t thread;
		#endif
	
		//! This function works on the quiries.
		/*!
		The function read a request from queue.
		Checks if a lock blocks the request.
		If any we will wait.
		And finally the request will be moved into \c stxxl::map_internal::btree_request
		*/
		static void* worker (void *arg)
		{
			// ******************************************************************
			// a thrae will be initialize
			// ******************************************************************
			
			map_queue *pthis = static_cast<map_queue*>(arg);
			smart_ptr< btree_request<btree> > sp_bt_req;
			smart_ptr< map_request<map> > sp_map_req;
			#ifndef STXXL_BOOST_THREADS
			stxxl_nassert (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
			stxxl_nassert (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));
			#endif

			// ******************************************************************
			// forever
			// ******************************************************************
			for (;;)
			{
				pthis->sem--;

				// ****************************************************************
				// get next request
				// ****************************************************************
				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock WorkerLock(pthis->worker_mutex);
				#else
				pthis->worker_mutex.lock ();
				#endif
				STXXL_ASSERT( !pthis->worker_queue.empty () );
				sp_map_req = pthis->worker_queue.front ();
				pthis->worker_queue.pop ();
				#ifdef STXXL_BOOST_THREADS
				WorkerLock.unlock();
				#else
				pthis->worker_mutex.unlock ();
				#endif


				// ****************************************************************
				// check locks
				// ****************************************************************

				lock_type* p_lock = (lock_type*) sp_map_req->_sp_int_request->_p_lock;

				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(pthis->lock_mutex);
				#else
				pthis->lock_mutex.lock();
				#endif
				
				STXXL_ASSERT( p_lock );

				if( pthis->lock_table.size() )
				{
					lock_table_iterator iter = pthis->lock_table.begin() ;
					while( iter != pthis->lock_table.end())
					{
						lock_type* p = *iter;
						if ( p && p->lock( *p_lock ))
						{
							lock_table_iterator h = iter;
							pthis->lock_table.erase(h);
							delete p;
						}
						else iter++;
					}
				}
				pthis->lock_table.push_back(p_lock);
				#ifdef STXXL_BOOST_THREADS
				Lock.unlock();
				#else
				pthis->lock_mutex.unlock();
				#endif

				// ****************************************************************
				// the request will be moved into the btree_queue
				// ****************************************************************
				sp_map_req->_sp_int_request->weakup();
				
				sp_map_req.release();
			}
			return NULL;
		}
	};
//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE


#endif
