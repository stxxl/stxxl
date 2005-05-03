/***************************************************************************
                          btree_request.h  -  description
                             -------------------
    begin                : Son Mai 2 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file btree_request.h
//! \brief Implemets the btree requests for the B-Tree implemantation for \c stxxl::map container.


#ifndef ___BTREE_REQUEST__H___
#define ___BTREE_REQUEST__H___

#include "map.h"
#include "btree_completion_handler.h"


__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	class btree_request_base;
	template<typename Btree> class btree_request;
	template<typename Btree> class btrequest_dump;
	template<typename Btree> class btrequest_const_next;
	template<typename Btree> class btrequest_erase;
	template<typename Btree> class btrequest_erase_key;
	template<typename Btree> class btrequest_erase_interval;
	template<typename Btree> class btrequest_insert;
	template<typename Btree> class btrequest_clear;
	template<typename Btree> class btrequest_valid;
	template<typename Btree> class btrequest_size;
	template<typename Btree> class btrequest_next;
	template<typename Btree> class btrequest_prev;
	template<typename Btree> class btrequest_const_prev;
	template<typename Btree> class btrequest_const_begin;
	template<typename Btree> class btrequest_begin;
	template<typename Btree> class btrequest_search;
	template<typename Btree> class btrequest_lower_bound;
	template<typename Btree> class btrequest_find;
	template<typename Btree> class btrequest_upper_bound;
	template<typename Btree> class btrequest_equal_range;
	template<typename Btree> class btrequest_const_search;
	template<typename Btree> class btrequest_const_upper_bound;
	template<typename Btree> class btrequest_const_find;
	template<typename Btree> class btrequest_const_lower_bound;
	template<typename Btree> class btrequest_const_equal_range;
	template<typename Btree> class btrequest_count;
	template<typename Btree,typename _InputIterator> class btrequest_bulk_insert;

	//! An interface for requests for \c stxxl::map_internal::btree.
	class btree_request_base : public ref_counter<btree_request_base>
	{

		friend class smart_ptr<btree_request_base>;

	protected:
		#ifdef STXXL_BOOST_THREADS
		boost::mutex waiters_mutex;
		#else
		mutex waiters_mutex;
		#endif
		std::set < onoff_switch * > waiters;
		state _state;
		btree_completion_handler on_complete;
		void completed() { on_complete( this, _p_lock ); }

	public:

		map_lock_base*              _p_lock;
		virtual int serve () = 0;
		virtual void weakup() = 0;
		
		bool add_waiter (onoff_switch * sw)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(waiters_mutex);
			#else
			waiters_mutex.lock ();
			#endif
			if (poll ()) // request already finished
			{
				#ifndef STXXL_BOOST_THREADS
				waiters_mutex.unlock ();
				#endif
				return true;
			}
			waiters.insert (sw);
			#ifndef STXXL_BOOST_THREADS
			waiters_mutex.unlock ();
			#endif
			return false;
		};

		void delete_waiter (onoff_switch * sw)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(waiters_mutex);
			waiters.erase (sw);
			#else
			waiters_mutex.lock ();
			waiters.erase (sw);
			waiters_mutex.unlock ();
			#endif
		}

		int nwaiters () // returns number of waiters
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(waiters_mutex);
			return waiters.size ();
			#else
			waiters_mutex.lock ();
			int size = waiters.size ();
			waiters_mutex.unlock ();
			return size;
			#endif
		}


public:
		enum state_type
		{
			WAIT4DATA = 0,
			OP = 1,
			DONE = 2,
			READY2DIE = 3,
		};

		enum
		{
			CLEAR = 0,
			BEGIN = 1,
			CONST_BEGIN = 2,
			LOWER_BOUND = 3,
			CONST_LOWER_BOUND = 4,
			UPPER_BOUND = 5,
			CONST_UPPER_BOUND = 6,
			FIND = 7,
			CONST_FIND = 8,
			NEXT = 9,
			CONST_NEXT = 10,
			PREV = 11,
			CONST_PREV = 12,
			EQUAL_RANGE = 13,
			CONST_EQUAL_RANGE = 14,
			INSERT = 15,
			BULK_INSERT = 16,
			ERASE = 17,
			ERASE_KEY = 18,
			ERASE_INTERVAL = 19,
			SIZE = 20,
			VALID = 21,
			DUMP = 22,
		};

		
		btree_request_base(
			btree_completion_handler on_compl = default_btree_completion_handler() ) :
			on_complete( on_compl ) {}


		virtual ~btree_request_base(){}
		virtual void set_to( int state ) { _state.set_to( state ); }
		virtual int cur_state()          { return _state(); }
		virtual void wait ()             { _state.wait_for( READY2DIE ); };
		virtual bool poll()              { return ( _state () >= DONE); };
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	template<typename Btree>
	class btree_request : public btree_request_base
	{
		friend class smart_ptr<btree_request>;

	protected:

		typedef Btree                             btree_type;
		typedef typename Btree::worker_queue_type worker_queue_type;
		typedef typename Btree::iterator          btree_iterator_type;
		typedef typename Btree::key_type          key_type;
		typedef typename Btree::_Node             node_type;
		typedef typename Btree::_Leaf             leaf_type;
		typedef typename Btree::node_ptr          node_ptr;
		typedef typename Btree::leaf_ptr          leaf_ptr;

	public:
		int                                       _type;
		btree_request( int  type, btree_completion_handler on_compl ):
			btree_request_base( on_compl ), _type( type ) {}
		virtual ~btree_request() { STXXL_ASSERT( _state() == READY2DIE ); };

		virtual void unlock(){ _p_lock->unlock(); }
		virtual void weakup(){ Btree::_btree_queue.add_request( this ); }
		virtual void call() = 0;
		virtual int serve()
		{
			call();
			int st = _state();
			if( st == DONE )
			{
				completed();
				_state.set_to(READY2DIE) ;
				return READY2DIE;
			}
			return st;
		}
  };

	//! A class for requests for \c stxxl::map_internal::btree.
	template<typename Btree>
	class btrequest_dump : public btree_request<Btree>
	{
		typedef typename Btree::_Leaf           leaf_type;
		typedef typename Btree::_Node           node_type;
		typedef typename Btree::bid_type        bid_type;

		smart_ptr<Btree>            _sp_btree;

	public:

		int                         _height;
		smart_ptr<leaf_type>        _sp_leaf;
		smart_ptr<node_type>        _sp_node;
		bid_type                    _bid_first_next;

		virtual void call() {  _sp_btree->dump( this ); }
		btrequest_dump( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( DUMP, on_compl),
			_sp_btree(p_btree), _height(-1){};
		virtual ~btrequest_dump(){}
	};


	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_erase : public btree_request<Btree>
	{
	public:
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;
		typedef typename Btree::bid_type bid_type;

		enum
		{
			PHASE_ERASE = 0,
			PHASE_NEXT_AND_ERASE = 1,
			PHASE_REPLACE_KEY = 2,
			PHASE_NEXT_AND_REPLACE_KEY = 3,
			PHASE_SET_NEXT_NEXT = 4,
		};

		smart_ptr<Btree> _sp_btree;

		int            _phase;
		int            _height;
		int            _height2;
		int            _erase_in_tree;

	// IN:
		iterator* _p_iter;

	// **********************************************
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;
		smart_ptr<node_type> _sp_cur_node2;
		smart_ptr<leaf_type> _sp_cur_leaf2;
		bid_type _bid;
	// **********************************************


		enum
		{
			NO_ERASED 			= 0 ,
			NO_ERASED_NEXT	= 1 ,
			NO_ERASED_PREV	= 2 ,
		};

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
			_sp_cur_node2.release();
			_sp_cur_leaf2.release();
		}
		virtual void call() { _sp_btree->aerase( this ); }
		btrequest_erase( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( ERASE, on_compl),
			_sp_btree( p_btree ),
			_phase( PHASE_ERASE ),
			_height( -1 ),
			_height2( 0 ),
			_erase_in_tree( NO_ERASED ),
			_p_iter( NULL )
			 {
			 };

		~btrequest_erase()
		{
		}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_erase_key : public btree_request<Btree>
	{
	public:
		typedef typename Btree::key_type key_type;
		typedef typename Btree::size_type size_type;
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;
		typedef typename Btree::bid_type bid_type;

		enum
		{
			PHASE_SEARCH = 5,
			PHASE_MOVE_NEXT = 6,
			PHASE_MOVE_PREV = 7,
			PHASE_ERASE = 8,
			PHASE_NEXT_AND_ERASE = 9,
			PHASE_REPLACE_KEY = 10,
			PHASE_NEXT_AND_REPLACE_KEY = 11,
			PHASE_SET_NEXT_NEXT = 12,
		};

		smart_ptr<Btree> _sp_btree;

		int            _phase;
		int            _height;
		int            _height2;
		int            _erase_in_tree;
	// IN:
		key_type _key;

	// OUT:
  	size_type* _p_size;
	
		
		// **********************************************
		smart_ptr<iterator>  _sp_iter;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;
		smart_ptr<node_type> _sp_cur_node2;
		smart_ptr<leaf_type> _sp_cur_leaf2;
		bid_type _bid;
		// **********************************************

		enum
		{
			NO_ERASED 			= 0 ,
			NO_ERASED_NEXT	= 1 ,
			NO_ERASED_PREV	= 2 ,
		};

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
			_sp_cur_node2.release();
			_sp_cur_leaf2.release();
		}
		virtual void call() { _sp_btree->aerase( this ); }
		btrequest_erase_key( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( ERASE_KEY, on_compl),
			_sp_btree( p_btree ),
			_phase( PHASE_SEARCH ),
			_height( -1 ),
			_height2( 0 ),
			_erase_in_tree( NO_ERASED ),
			_p_size( NULL )
			 {
			 };

		~btrequest_erase_key()
		{
		}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_erase_interval : public btree_request<Btree>
	{
	public:
		typedef typename Btree::node_store_type node_store_type;
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;
		typedef typename Btree::bid_type bid_type;

		smart_ptr<Btree> _sp_btree;

		enum
		{
			PHASE_ERASE = 13,
			REORDER_NODES = 14,
			REORDER_WITH_NEXT = 15,
			REORDER_WITH_PREV = 16,
			PHASE_SET_NEXT_NEXT_AFTER_REORDER = 17,
			PHASE_SET_NEXT_NEXT = 18,
			CLEAR_UP = 19,
			MOVE_NEXT_AFTER_REODER = 20
		};

		int            _phase;
		int            _height;
		int            _height2;


	// IN:
		iterator* _p_first;
		iterator* _p_last;

	// **********************************************
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;
		smart_ptr<node_type> _sp_cur_node2;
		smart_ptr<leaf_type> _sp_cur_leaf2;

		std::vector<node_store_type> _node_vector;
		bid_type                     _bid;

	// **********************************************

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
			_sp_cur_node2.release();
			_sp_cur_leaf2.release();
		}

		virtual void call() { _sp_btree->aerase( this ); }
		btrequest_erase_interval( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( ERASE_INTERVAL, on_compl),
			_sp_btree( p_btree ),
			_phase( PHASE_ERASE ),
			_height( -1 ),
			_p_first( NULL ),
			_p_last( NULL )
			 {
			 };

		~btrequest_erase_interval()
		{
		}
	};

	template<typename Btree>
	std::ostream& operator << ( std::ostream& str, const btrequest_erase_interval<Btree>& x )
	{
		x.operator <<( str ); return str;
	}

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_insert : public btree_request<Btree>
	{
	public:
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;
		typedef typename Btree::store_type store_type;
		typedef typename Btree::node_store_type node_store_type;
		typedef typename Btree::bid_type bid_type;

		enum
		{
			PHASE_SEARCH = 21,
			PHASE_MOVE_NEXT = 22,
			PHASE_MOVE_PREV = 23,
			PHASE_INSERTING = 24,
			PHASE_REPLACE_BID_IN_NEXT_NODE = 25,
		};

		smart_ptr<Btree>     _sp_btree;
		iterator**           _pp_iterator;
		bool*                _p_bool;
		store_type           _pair;
		int                  _phase;
		int                  _height;
		int                  _height2;
		node_store_type      _node_pair;
		bid_type             _bid;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}
		virtual void call()
		{
			_sp_btree->ainsert( this );
		}
		btrequest_insert( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( INSERT, on_compl),
			_sp_btree( p_btree ),
			_pp_iterator( NULL ),
			_p_bool( NULL ),
			_phase(PHASE_SEARCH),
			_height( -1 ) {}
		virtual ~btrequest_insert() {}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree,typename _InputIterator>
	class btrequest_bulk_insert : public btree_request<Btree>
	{
	public:
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;
		typedef typename Btree::store_type store_type;
		typedef typename Btree::node_store_type node_store_type;
		typedef typename Btree::bid_type bid_type;
		
		enum
		{
			PHASE_SEARCH = 26,
			PHASE_MOVE_NEXT = 27,
			PHASE_MOVE_PREV = 28,
			PHASE_INSERTING = 29,
			PHASE_REPLACE_BID_IN_NEXT_NODE = 30,
		};

		smart_ptr<Btree>     _sp_btree;
		smart_ptr<iterator>  _sp_iterator;
		int                  _phase;
		int                  _height;
		int                  _height2;
		node_store_type      _node_pair;
		bid_type             _bid;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;

		_InputIterator _first;
		_InputIterator _last;

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
			_sp_iterator.release();
		}
		virtual void call() { _sp_btree->abulk_insert( this ); }
		btrequest_bulk_insert( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( BULK_INSERT, on_compl),
			_sp_btree( p_btree ),
			_phase(PHASE_SEARCH),
			_height( -1 ) {}
		virtual ~btrequest_bulk_insert() {}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_clear : public btree_request<Btree>
	{
	public:
		typedef typename Btree::_Node node_type;

		smart_ptr<Btree> _sp_btree;
		smart_ptr<node_type> *_p_sp_node;
		int _height;
		int _phase;

		enum
		{
			PHASE_VERTICAL = 31,
			PHASE_HORIZONTAL = 32,
		};

		void DisposeParam() { if( _p_sp_node ) delete[] _p_sp_node; }
		void call() { _sp_btree->aclear( this ); }

		btrequest_clear( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( CLEAR, on_compl),
			_sp_btree( p_btree ),
			_p_sp_node( NULL ),
			_height( -1 ),
			_phase( PHASE_VERTICAL ) {}
		virtual ~btrequest_clear() {}
	};


	//! A class for requests for \c stxxl::map_internal::btree.
	template<typename Btree>
	class btrequest_valid : public btree_request<Btree>
	{
	public:
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Node leaf_type;
		typedef typename Btree::key_type key_type;

		smart_ptr<Btree> _sp_btree;
		smart_ptr<node_type> *_p_sp_node;
		smart_ptr<leaf_type> _sp_leaf;
		int _key;
		int _height;
		int _phase;

		bool _valid;

		enum
		{
			PHASE_VERTICAL = 33,
			PHASE_HORIZONTAL = 34,
		};

		void DisposeParam() {}
		void call() { _sp_btree->avalid( this ); }

		btrequest_valid( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( CLEAR, p_btree, on_compl),
			_height( -1 ),
			_phase( PHASE_VERTICAL ),
			_valid(true) {}
		virtual ~btrequest_valid() {}
	};


	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_size : public btree_request<Btree>
	{
		typedef typename Btree::size_type size_type;

	public:
		const Btree* _p_btree;
		size_type* _p_size;

		void call() { _p_btree->asize( this ); }

		btrequest_size( const Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( SIZE, on_compl), _p_btree(p_btree), _p_size( NULL ) {}
		virtual ~btrequest_size() {}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see stxxl::map_iternal::const_map_iterator
	*/
	template<typename Btree>
	class btrequest_const_next : public btree_request<Btree>
	{
	public:
		typedef typename Btree::const_iterator  const_iterator;
		typedef typename Btree::_Leaf           leaf_type;

		const Btree*                _p_btree;
		const_iterator**            _pp_const_iterator;
		bool                        _next;
		smart_ptr<leaf_type>        _sp_cur_leaf;


		void DisposeParam() { _sp_cur_leaf.release(); }
		virtual void call() { _p_btree->anext( this ); }

		btrequest_const_next( const Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( CONST_NEXT, on_compl),
			_p_btree( p_btree ),
			_pp_const_iterator( NULL ),
			_next( false ) {};

		virtual ~btrequest_const_next(){}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see stxxl::map_iternal::map_iterator
	*/
	template<typename Btree>
	class btrequest_next : public btree_request<Btree>
	{
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;

	public:

		smart_ptr<Btree> _sp_btree;
		iterator** _pp_iterator;
		int _height;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}

		virtual void call() { _sp_btree->anext( this ); }
		btrequest_next( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( NEXT, on_compl),
			_sp_btree( p_btree ),
			_pp_iterator( NULL ),
			_height( -1 ) {}
		virtual ~btrequest_next() {}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see stxxl::map_iternal::map_iterator
	*/
	template<typename Btree>
	class btrequest_prev : public btree_request<Btree>
	{
	public:
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;

		smart_ptr<Btree> _sp_btree;
		int _height;
		iterator** _pp_iterator;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}
  	virtual void call() { _sp_btree->aprev( this ); }

		btrequest_prev( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( PREV, on_compl),
			_sp_btree( p_btree ),
			_height( -1 ),
			_pp_iterator( NULL ) {}
		virtual ~btrequest_prev() {}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see stxxl::map_iternal::const_map_iterator
	*/
	template<typename Btree>
	class btrequest_const_prev : public btree_request<Btree>
	{
	public:
		typedef typename Btree::const_iterator const_iterator;
		typedef typename Btree::_Leaf          leaf_type;

		const Btree*                _p_btree;
		const_iterator**            _pp_const_iterator;
		bool                        _next;
		smart_ptr<leaf_type>        _sp_cur_leaf;

		void DisposeParam() { _sp_cur_leaf.release(); }
		virtual void call() { _p_btree->aprev( this ); }
	  btrequest_const_prev( const Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( CONST_PREV, on_compl),
			_p_btree( p_btree ),
			_pp_const_iterator( NULL ),
			_next( false )
		{}
		virtual ~btrequest_const_prev() {}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_const_begin : public btree_request<Btree>
	{
	public:
		typedef typename Btree::const_iterator const_iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;

		const Btree* _p_btree;
		const_iterator** _pp_const_iterator;
		int _height;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}
		virtual void call() { _p_btree->abegin( this ); }

		btrequest_const_begin( const Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( CONST_BEGIN, on_compl),
			_p_btree( p_btree ),
			_pp_const_iterator( NULL ),
			_height( -1 ) {}

		virtual ~btrequest_const_begin(){}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_begin : public btree_request<Btree>
	{

	public:
		typedef typename Btree::iterator iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;

		smart_ptr<Btree> _sp_btree;
		iterator** _pp_iterator;
		int _height;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}
		virtual void call()
		{
			_sp_btree->abegin( this );
		}

		btrequest_begin( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( BEGIN, on_compl),
			_sp_btree( p_btree ),
			_pp_iterator( NULL ),
			_height( -1 )
		{
		}
			
		virtual ~btrequest_begin()
		{
		}
	};

	/********************************************************************
		SEARCHER
	*********************************************************************/

	
	//! A base class for search requests for \c stxxl::map_internal::btree.
	template<typename Btree>
	class btrequest_search : public btree_request<Btree>
	{
	public:
		typedef typename Btree::iterator  iterator;
		typedef typename Btree::_Node     node_type;
		typedef typename Btree::_Leaf     leaf_type;
		typedef typename Btree::key_type  key_type;

		smart_ptr<Btree> _sp_btree;
		iterator** _pp_iterator;
		key_type _key;
		int _height;
		int _height2;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;
		int _phase;

		enum
		{
			PHASE_SEARCH = 35,
			PHASE_MOVE_NEXT = 36,
			PHASE_MOVE_PREV = 37,
		};

		void DisposeParam()
		{
			_height = -1;
			_phase = PHASE_SEARCH;
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}
		virtual void call() = 0;

		btrequest_search( int type, Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( type, on_compl),
			_sp_btree( p_btree ),
			_pp_iterator( NULL ),
			_height( -1 ),
			_height2( -1 ),
			_phase(PHASE_SEARCH)
		{};

		virtual ~btrequest_search() {}
	};

	/**************************************************************************/

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_lower_bound : public btrequest_search<Btree>
	{
	public:
		virtual void call()
		{
			if ( _sp_btree->alower_bound( this ) )
			{
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_lower_bound( Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_search<Btree>( LOWER_BOUND, p_btree, on_compl) {};

		virtual ~btrequest_lower_bound() {};
	};

	/**************************************************************************/

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_find : public btrequest_search<Btree>
	{
	public:
		virtual void call()
		{
			if( _sp_btree->afind( this ) )
			{
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_find( Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_search<Btree>( FIND, p_btree, on_compl)
		{};
		
		virtual ~btrequest_find() {}
	};

	/**************************************************************************/

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_upper_bound : public btrequest_search<Btree>
	{
	public:
		virtual void call()
		{
			if( _sp_btree->aupper_bound( this ) )
			{
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_upper_bound( Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_search<Btree>( UPPER_BOUND, p_btree, on_compl) {}
		virtual ~btrequest_upper_bound() {}
	};

	/**************************************************************************/

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_equal_range : public btrequest_search<Btree>
	{
	public:
		typedef typename Btree::iterator iterator;

		iterator** _pp_lower_iterator;
		iterator** _pp_upper_iterator;
		bool upper;

		virtual void call()
		{
			if( !upper ) _pp_iterator = _pp_lower_iterator;
			if( !upper && _sp_btree->alower_bound( this ) )
			{
				DisposeParam();
				upper = true;
			}

			if( upper ) _pp_iterator = _pp_upper_iterator;
			if( upper && _sp_btree->aupper_bound( this ) )
			{
				_pp_upper_iterator = _pp_iterator;
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_equal_range( Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_search<Btree>( EQUAL_RANGE, p_btree, on_compl),
			_pp_lower_iterator( NULL ),
			_pp_upper_iterator( NULL ),
			upper( false ) {}

		virtual ~btrequest_equal_range() {}
	};

	/*************************************************************
	  CONST SEARCHER
	**************************************************************/

	//! A base class for const search requests for \c stxxl::map_internal::btree.
	template<typename Btree>
	class btrequest_const_search : public btree_request<Btree>
	{
	public:
		typedef typename Btree::const_iterator const_iterator;
		typedef typename Btree::_Node node_type;
		typedef typename Btree::_Leaf leaf_type;
		typedef typename Btree::key_type key_type;

		const Btree* _p_btree;
		const_iterator** _pp_const_iterator;
		key_type _key;
		int _height;
		// int _height2;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;
		int _phase;

		enum
		{
			PHASE_SEARCH = 38,
			PHASE_MOVE_NEXT = 39,
		};

		void DisposeParam()
		{
			_height = -1;
			_phase = PHASE_SEARCH;
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}
		virtual void call() = 0;

		btrequest_const_search( int type, const Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( type, on_compl),
			_p_btree( p_btree),
			_pp_const_iterator( NULL ),
			_height( -1 ),
			_phase(PHASE_SEARCH) {}
		virtual ~btrequest_const_search() {}
	};

	/***********************************************************************************/

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_const_upper_bound : public btrequest_const_search<Btree>
	{
	public:
		virtual void call()
		{
			if ( _p_btree->aupper_bound( this ) )
			{
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_const_upper_bound( const Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_const_search<Btree>( CONST_UPPER_BOUND, p_btree, on_compl) {}

		virtual ~btrequest_const_upper_bound() {}
	};

	/**************************************************************************/

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_const_find : public btrequest_const_search<Btree>
	{
	public:
		virtual void call()
		{
			if ( _p_btree->afind( this ) )
			{
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_const_find( const Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_const_search<Btree>( CONST_FIND, p_btree, on_compl)	{};

		virtual ~btrequest_const_find() {}
	};

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_const_lower_bound : public btrequest_const_search<Btree>
	{
	public:
		virtual void call()
		{
			if ( _p_btree->alower_bound( this ) )
			{
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_const_lower_bound( const Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_const_search<Btree>( CONST_LOWER_BOUND, p_btree, on_compl) {};

		virtual ~btrequest_const_lower_bound() {};
	};

	/**************************************************************************/

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_const_equal_range : public btrequest_const_search<Btree>
	{
	public:
		typedef typename Btree::const_iterator const_iterator;
		
		const_iterator** _pp_const_lower_iterator;
		const_iterator** _pp_const_upper_iterator;
		bool upper;

		virtual void call()
		{
			if ( !upper ) _pp_const_iterator = _pp_const_lower_iterator ;
			if ( !upper && _p_btree->alower_bound( this ) )
			{
				DisposeParam();
				upper = true;
			}

			if ( upper ) _pp_const_iterator = _pp_const_upper_iterator ;
			if ( upper && _p_btree->aupper_bound( this ) )
			{
				_pp_const_upper_iterator = _pp_const_iterator;
				DisposeParam();
				set_to( btree_request<Btree>::DONE );
			}
		}

		btrequest_const_equal_range( const Btree* p_btree, btree_completion_handler on_compl ) :
			btrequest_const_search<Btree>( CONST_EQUAL_RANGE, p_btree, on_compl),
			_pp_const_lower_iterator( NULL ),
			_pp_const_upper_iterator( NULL ),
			upper( false ) {};

		virtual ~btrequest_const_equal_range() {};
	};


	//***************************************************************

	//! A class for requests for \c stxxl::map_internal::btree.
	/*!
	\see \c stxxl::map_iternal::map_impl
	*/
	template<typename Btree>
	class btrequest_count : public btree_request<Btree>
	{
	public:
		typedef typename Btree::const_iterator const_iterator;
		typedef typename Btree::_Node          node_type;
		typedef typename Btree::_Leaf          leaf_type;
		typedef typename Btree::key_type       key_type;

		enum
		{
			PHASE_SEARCH = 40,
			PHASE_MOVE_NEXT = 41,
		};

		const_iterator** _pp_const_iterator;
		key_type _key;
		int _height;
		int _height2;
		smart_ptr<node_type> _sp_cur_node;
		smart_ptr<leaf_type> _sp_cur_leaf;
		int _phase;

		void DisposeParam()
		{
			_sp_cur_node.release();
			_sp_cur_leaf.release();
		}
		virtual void call()
		{
			_sp_btree->acount( this );
		}

		btrequest_count( Btree* p_btree, btree_completion_handler on_compl ) :
			btree_request<Btree>( COUNT, p_btree, on_compl),
			_pp_const_iterator( NULL ),
			_height( -1 ),
			_phase(PHASE_SEARCH) {};

		virtual ~btrequest_count() {};
	};
	//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE

#endif
