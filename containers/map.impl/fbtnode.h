/***************************************************************************
                          fbtnode.h  -  description
                             -------------------
    begin                :  Feb 22 2005
    copyright            : (C) 2005 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file fbtnode.h
//! \brief Implemets a cointener \c stxxl::map_internal::btnode nodes for the implementation of \c stxxl::map container.

#ifndef __STXXL_BTNODE_H
#define __STXXL_BTNODE_H

#include "map.h"
#include "btnode_manager.h"
#include "btnode_iterator.h"

__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	//! \brief Standard completion handler for read requests for a \c stxxl::map_internal::btnode.
	template< typename Node >
	struct btnode_io_completion_handler_read
	{
		smart_ptr<Node> _sp_node;

		btnode_io_completion_handler_read( Node *p_node ) : _sp_node( p_node ){}
		~btnode_io_completion_handler_read(){}
		btnode_io_completion_handler_read( const btnode_io_completion_handler_read& x ) : _sp_node( x._sp_node ){}

		void operator() (request *req )
		{
			_sp_node->complete_queue();
			_sp_node.release();
		};
	};


  //! \brief Standard completion handler for write requests for a btnode.
	template< typename Node, unsigned SIZE_>
	struct btnode_io_completion_handler_write
	{
		::stxxl::map_internal::smart_ptr<Node> _sp_node;
		async_btnode_manager_base<SIZE_>* _p_manager;

		btnode_io_completion_handler_write( Node *p_node, async_btnode_manager_base<SIZE_> *p_manager ) :
			_sp_node( p_node ), _p_manager( p_manager ){}

		~btnode_io_completion_handler_write(){}

		btnode_io_completion_handler_write( const btnode_io_completion_handler_write& x ) :
			_sp_node( x._sp_node ), _p_manager( x._p_manager ){}

		void operator() (request *req )
		{
			_p_manager->remove( _sp_node->bid() );
			_sp_node.release();
		};
	};

	class btnode_manager_entry
	{
	}; 

	
  //! \brief Class implements the nodes and leafs container of the B-tree implementing of the map.
	/*!
	Template parameters:
   * - \c Key_ The btnodes's key type
   * - \c Data_ The btnodes's data type
   * - \c Compare_ The key comparsion function
   * - \c SIZE_ External block size in bytes, default is 4096 kbytes
   * - \c unique_ A flag for map/multimap implementation (not jet implement).
   * - \c CACHE_SIZE_ A size we prefer the node cache for this node type will be grow.

   All public functions (with no const iterator) are thread secure.
   A node consists of a field of pairs of keys also in addition belonging values.
   Each node possesses a Referen to the predecessor and successor (so far they are present).
   Each node knows the number of elements stored there.
   For lookup in the node used binary search.
   Therefore the search functions have logarithmic running time.
   For the semantic of interface see documentation of the STL std::map. */
	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	class btnode : public ref_counter<btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> >
	{

	
public:

	enum
	{
		// enums for state of a node

		READING = 0,       // after creating with a valid BID
		READY = 1,         // after writing or reading
		DONE = 2,          // after served all requsts after READY
		WRITING = 3,       // writing
		DELETED = 4,       // after writing by (DELTING)
	};

	enum
	{
		// enums for deleting operations

		SIMPLY_ERASED 	= 1 ,  
		MERGED_PREV 		= 2 ,
		MERGED_NEXT 		= 3 ,
		STEALED_PREV		= 4 ,
		STEALED_NEXT		= 5 ,
	};

	private:

		// private types
		typedef btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> _Self;
		typedef smart_ptr<btree_request_base> btree_request_base_ptr;

		// private memebers
		std::queue<btree_request_base_ptr>* _p_request_queue;
		request_ptr                         _sp_io_request;
		mutex                               _node_mutex;
		state                               _state;


	public:

		// public types

		typedef 	stxxl::BID<SIZE_>							bid_type;
		typedef 	Key_													key_type;
		typedef 	Data_													mapped_type;
		typedef 	std::pair<const Key_, Data_>	value_type;
		typedef 	std::pair<Key_, Data_>				store_type;
		typedef 	Compare_											key_compare;
		typedef 	value_type&										reference;
		typedef 	const value_type&							const_reference;
		typedef 	_SIZE_TYPE										size_type;
		typedef 	_SIZE_TYPE										difference_type;
		typedef 	value_type*										pointer;
		typedef 	const value_type*							const_pointer;

		// public members

		//! A static member will be used as for function stxxl::map::end.
		static _Self nil;             // an instanc represent a Nil the end of tree or map
#ifdef STXXL_DEBUG_ON
		bool 	is_nil;                 // a flag for this node ( for debuging only )
#endif

		// class for compare the keys and values
		class value_compare
		{
			protected:
				Compare_  _comp;
				value_compare() {};
				value_compare(Compare_ comp) : _comp(comp) {};

			public:
				bool 		operator()	(const value_type& x, const value_type& y) const
					{ return _comp(x.first, y.first); }

				bool 		operator()	(const key_type& x, const key_type& y) const
					{ return _comp(x, y); }

			friend class btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>;
		};

		key_compare     key_comp() const     { return _value_compare._comp; }
		value_compare   value_comp() const   { return _value_compare; }

	private:

		value_compare _value_compare; // a member of type compare

	private:

		// class form external memory
		class node_store
		{
			public:
				size_type 					_cur_count;
				stxxl::BID<SIZE_>		_bid_prev;
				stxxl::BID<SIZE_>		_bid_next;
				char								_Array[ SIZE_ - sizeof(size_type) - 2* sizeof(stxxl::BID<SIZE_>) ];
			public:
				node_store() :
				_cur_count(0) {}
				~node_store() {}
				void dump()
				{
					STXXL_MSG( _cur_count );
					STXXL_MSG( _bid_prev );
					STXXL_MSG( _bid_next );
					for( unsigned i = 0 ; i < sizeof(_Array)/sizeof(char); i++ )
					{
						std::cout << (char) ( _Array[i] / 16 + 48 ) << (char) ( _Array[i] % 16 + 48 ) ;
					}
					std::cout << "\n";
				}
		};

		// type for the external memory.
		typedef typed_block<SIZE_,node_store> block_type;


		block_type 										*_p_block;  // a pointer to the space,
																							// wich will be read/write in external memory.
		value_type										*_p_values; // a helper for access to the members of node_store.

		stxxl::BID<SIZE_>							_bid;       // a address to external memory for current block.
		bool													_dirty;     // a flag means if the node have to be writen befor deleting.
//		int														_n_lum;
		static size_type							_max_count;	// the maximal number of entries in a node.

	public:

		#ifdef STXXL_BTREE_STATISTIC

		// members for the map statistic
		static bt_counter					_temp_node_stat;
		static bt_counter					_all_node_stat;
		static bt_counter					_reads_stat;
		static bt_counter					_writes_stat;

		#endif

		// ---------------------------------------------------------------------------------------------------
		// Iterators
		// ---------------------------------------------------------------------------------------------------

	public:

		typedef btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>				iterator;
		typedef const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> 	const_iterator;

		// -------------------------------------------------------------------------------
		// Constructors for btnode
		// -------------------------------------------------------------------------------

		//! constructor
		btnode( bool _nil = false ) : _state( DONE ), _dirty( true ) // , _n_lum( 0 )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_temp_node_stat++;
			#endif

			if ( _nil ) add_ref();

			#ifdef STXXL_DEBUG_ON
			is_nil = _nil;
			#endif

			_initialize();
			STXXL_VERBOSE3( "btnode\t\t" << this <<": construction" );
		}


		//! constructor
		btnode( BID<SIZE_> bid ) : _state( READING ), _bid( bid ), _dirty( false ) // , _n_lum( 0 )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_temp_node_stat++;
			#endif

			#ifdef STXXL_DEBUG_ON
			is_nil = false;
			#endif

			_initialize();
			STXXL_VERBOSE3( "btnode\t\t" << this <<": construction, bid = " << bid );
		}

		//! constructor
		btnode(const Compare_& comp) : _state( DONE ), _value_compare( comp ), _dirty( true ) //, _n_lum( 0 )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_temp_node_stat++;
			#endif

			#ifdef STXXL_DEBUG_ON
			is_nil = false;
			#endif

			_initialize();
			STXXL_VERBOSE3( "btnode\t\t" << this <<": construction" );
		}

		//! constructor
		btnode(const btnode& node) : _state( DONE ), _value_compare( node._value_compare ), _dirty( true ) //, _n_lum( 0 )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_temp_node_stat++;
			#endif

			#ifdef STXXL_DEBUG_ON
			STXXL_ASSERT( 0 );
			is_nil = false;
			#endif

			_initialize();
			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( node._p_block );
			memcpy( _p_block->elem, node._p_block->elem, SIZE_ );
			STXXL_VERBOSE3( "btnode\t\t" << this <<": construction(copy)" );
		}

		//! constructor
		template <typename InputIterator_>
		btnode( InputIterator_ first, InputIterator_ last	) : _state( DONE ), _dirty( true ) // , _n_lum( 0 )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_temp_node_stat++;
			#endif

			#ifdef STXXL_DEBUG_ON
			is_nil = false;
			#endif

			_initialize();
			STXXL_VERBOSE3( "btnode\t\t" << this <<": construction" );
			insert( first, last );
		}

		//! constructor
		template <typename InputIterator_>
		btnode( InputIterator_ first, InputIterator_ last, const Compare_& comp ) : _state( DONE ), _value_compare( comp ), _dirty( true ) // , _n_lum( 0 )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_temp_node_stat++;
			#endif

			#ifdef STXXL_DEBUG_ON
			is_nil = false;
			#endif

			_initialize();
			STXXL_VERBOSE3( "btnode\t\t" << this <<": construction" );
			insert( first, last );
		}

		//! destructor
		~btnode()
		{
			_node_mutex.lock();
			_terminate();
			_node_mutex.unlock();
			STXXL_VERBOSE3( "btnode\t\t" << this <<": destruction" );

			#ifdef STXXL_BTREE_STATISTIC
			_temp_node_stat--;
			#endif

		}


		// -------------------------------------------------------
		// Helpfull functions
		// -------------------------------------------------------

		//! stes the dirty flag
		void set_dirty( bool val)
		{
			_node_mutex.lock();
			_dirty = val;
			_node_mutex.unlock();
		}

		//! gets the value of dirty flag
		bool dirty()
		{
			_node_mutex.lock();
			bool ret = _dirty;
			_node_mutex.unlock();
			return ret;
		}

		//! return an iterator to the begin of node
		iterator begin()
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			_Self *me = this;
			_node_mutex.unlock();
			return iterator( me, 0 );
		}

		//! return an iterator to the begin of node
		const_iterator begin() const
		{
			STXXL_ASSERT( _p_block );
			return const_iterator( this, 0 );
		}

		//! return an iterator to the element behin last entry of node
		const_iterator end() const
		{
			STXXL_ASSERT( _p_block );
			_Self *me = this;
			return const_iterator( me, _p_block->elem->_cur_count );
		}

		//! return an iterator to the element behin last entry of node
		iterator end()
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			_Self *me = this;
			_node_mutex.unlock();
			return iterator( me, _p_block->elem->_cur_count );
		}

		//! return an iterator to the last element of node
		iterator last()
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( _p_block->elem->_cur_count );
			_Self *me = this;
			_node_mutex.unlock();
			return iterator( me,_p_block->elem->_cur_count - 1);
		}

		//! after a node is readed brodcast all request, waiting for the node
		void complete_queue()
		{
			smart_ptr< btree_request_base> sp_req;

			_node_mutex.lock();

			STXXL_ASSERT( _state() == READING || _state() == WRITING );

			_state = READY;
			if ( _p_request_queue )
			{
				while( !_p_request_queue->empty () )
				{
					sp_req = _p_request_queue->front ();
					_p_request_queue->pop ();
					sp_req->weakup ();
				}
				delete _p_request_queue;
			}
			_p_request_queue = NULL;
			_state = DONE;
			_dirty = false;

			_node_mutex.unlock();
		}

		//! adds a new request to the node's requests queue
		request_ptr& add_request( btree_request_base* p_req )
		{
			smart_ptr<btree_request_base>  sp_req = p_req;

			_node_mutex.lock ();

			switch ( _state() )
			{
				case DELETED:
					STXXL_ASSERT( 0 );
					break;

				case WRITING:
				case READING:
					if (!_p_request_queue) _p_request_queue = new std::queue< smart_ptr < btree_request_base > >;
					_p_request_queue->push ( smart_ptr<btree_request_base>( p_req ) );
					sp_req->set_to( btree_request_base::WAIT4DATA );
					break;

				case READY:
				case DONE:
					sp_req->set_to( btree_request_base::OP );
					break;

				default:
					STXXL_ASSERT( 0 );
					break;
			}
			_node_mutex.unlock();

			return _sp_io_request;
		}

	private:

		// -----------------------------------------------------
		// private functions are not thread secure.
		// -----------------------------------------------------

		//! initialize a node
		void _initialize()
		{
			_p_request_queue = NULL;
			_p_block = new block_type;
			_p_values = (value_type*) _p_block->elem->_Array;
			clear();
		}

		//! terminste a node
		void _terminate()
		{
			// if( _dirty ) _write();
			if( _dirty && _bid.valid())
			{
				dump( std::cout );
			}
			STXXL_ASSERT( !_dirty || !_bid.valid());
			STXXL_ASSERT( _n_ref >= 0 );
			STXXL_ASSERT( _n_ref <= 0 || is_nil || _n_ref <= 1 && !_bid.valid() );

			if ( _p_request_queue ) delete _p_request_queue;
			if ( !_n_ref || !_bid.valid() ) delete _p_block; _p_block = NULL;
		}

		// internal lower_bound
		size_type _find_lower( size_type first, size_type last, const key_type& key ) const
		{
			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( first <= _p_block->elem->_cur_count );
			STXXL_ASSERT( last <= _p_block->elem->_cur_count );
			STXXL_ASSERT( first <= last );

			size_type cur_pos;
			while( first != last && first != _p_block->elem->_cur_count )
			{
				cur_pos = ( first + last - 1 ) / 2;
				if ( _value_compare( _p_values[ cur_pos ].first, key ) )
					first = ++cur_pos;
				else
				last = cur_pos;
			}
			if( last > max_size() ) return max_size();
			return last ;
		}

		// internal upper bound
		size_type _find_upper( size_type first, size_type last, const key_type& key ) const
		{
			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( first <= _p_block->elem->_cur_count );
			STXXL_ASSERT( last <= _p_block->elem->_cur_count );
			STXXL_ASSERT( first <= last );

			size_type cur_pos;
			while( last != first && last != 0 )
			{
				if ( !_value_compare( key, _p_values[ cur_pos = ( first + last - 1 ) / 2 ].first  ) )
					first = ++cur_pos;
				else
					last = cur_pos;
			}
			return last ;
		}

		// internal insert
		bool _insert( size_type pos, const value_type& pair )
		{
			STXXL_ASSERT( _p_block );

			size_type hpos 	= _p_block->elem->_cur_count;
			store_type*	ptr 	= ( store_type *) _p_values;

			while( hpos != pos ) *( ptr + hpos ) = *( ptr + --hpos );
			*( ptr + hpos ) = pair ;
			++_p_block->elem->_cur_count ;
			STXXL_ASSERT( _p_block->elem->_cur_count <= _max_count );
			_dirty = true;
			return true;
		}
	
		// internal delete
		bool _delete( size_type pos1, size_type pos2 )
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			store_type* ptr = (store_type*) _p_values;
			_p_block->elem->_cur_count -= pos2 - pos1;
			while( pos1 != _p_block->elem->_cur_count ) ptr[pos1++] = ptr[pos2++];
			_dirty = true;
			_node_mutex.unlock();
			return true;
		}

		// return the current reference counter for the node
		// Will be used by node caching.
		int IncLum()
		{
			_node_mutex.lock();
			int ret = _n_ref;
			_node_mutex.unlock();
			return ret;
		}

	public:

		// ---------------------------------------------------------------------------------
		// Operators
		// ---------------------------------------------------------------------------------

		// this function is not secure.
		// Should not be called.
		btnode&	operator=(const btnode& x)
		{
			STXXL_ASSERT( 0 );
			STXXL_VERBOSE2( "Copy node" );
			_dirty = true;
			insert( x.begin(), x.end() );
			return *this;
		}

		//! access to the entries of node
		mapped_type& operator [] ( const key_type& key )
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_store );
			size_type pos = _find_lower( 0, _p_store->_cur_count, key );
			_dirty = true;
			mapped_type& ret = _p_values[ pos ].second;
			_node_mutex.unlock();
			return ret;
		}

		//! access to a key
		key_type get_key_at( size_type pos )
		{
			_node_mutex.lock();
			key_type k = _p_values[ pos ].first;
			_node_mutex.unlock();
			return k;
		}

		// ****************************************************
		// compares two nodes
		// ****************************************************

		//! comparsion
		bool operator ==(const _Self & node) const
		{
			return size() == node.size() && std::equal( begin(), end(), node.begin() );
		}

		//! comparsion
		bool operator !=(const _Self & node) const
		{
			return ! operator==( node );
		}

		//! comparsion
		bool operator <(const _Self & node) const
		{
			return std::lexicographical_compare( begin(), end(), node.begin(), node.end() );
		}


    //! internal memory allocation
		void allocate_block( block_manager* p_block_manager )
		{
			_node_mutex.lock();
			_allocate_block( p_block_manager );
			_node_mutex.unlock();
		}

		//!  internal memory releasing
		void free_block( block_manager* p_block_manager )
		{
			_node_mutex.lock();
			_free_block( p_block_manager );
			_node_mutex.unlock();
		}

		//! relese external memory coresponds to the node
		static void free_block( BID<SIZE_> bid, block_manager* p_block_manager )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_all_node_stat--;
			#endif
			BIDArray < SIZE_ > bids ( 1 );
			bids[0] = bid;
			p_block_manager->delete_blocks( bids.begin (), bids.end () );
		}

		//! external memory read will be call
		request_ptr read()
		{
			_node_mutex.lock();
			_read();
			_node_mutex.unlock();

			_sp_io_request->wait();
			return _sp_io_request;
		}

		//! external memory write will be call
		request_ptr write ( async_btnode_manager_base<SIZE_> *p_manager )
		{
			_node_mutex.lock();
			request_ptr ret = _write( p_manager );
			_node_mutex.unlock();

			if ( ret.get() ) ret->wait();
			return ret;
		}


	private:

		// ---------------------------------------------------------------------------------
		// private I/O and block operations
		// ---------------------------------------------------------------------------------

		void _allocate_block( block_manager* p_block_manager )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_all_node_stat++;
			#endif
			BIDArray < SIZE_ > bids ( 1 );
			p_block_manager->new_blocks ( FR(), bids.begin (), bids.end () );
			_bid = bids[0];
		}

		void _free_block( block_manager* p_block_manager )
		{
			#ifdef STXXL_BTREE_STATISTIC
			_all_node_stat--;
			#endif
			BIDArray < SIZE_ > bids ( 1 );
			bids[0] = _bid;
			p_block_manager->delete_blocks( bids.begin (), bids.end () );
			_bid = bid();
		}

		void _read()
		{
			STXXL_ASSERT( !_dirty );
			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( _bid.valid() );

			_dirty = false;
			#ifdef STXXL_BTREE_STATISTIC
			_reads_stat++;
			#endif

			_state.set_to( READING );
			btnode_io_completion_handler_read<_Self> handler( this );
			_sp_io_request = _p_block->read( _bid, handler);
		}

		request_ptr _write( async_btnode_manager_base<SIZE_> *p_manager )
		{
			if( !_dirty )
			{
				if( _bid.valid() ) p_manager->remove( _bid );
				return NULL;
			}

			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( _bid.valid() );

			if( !_bid.valid() )
			{
				STXXL_ASSERT( 0 );
				_dirty = false;
				return NULL;
			};

			#ifdef STXXL_BTREE_STATISTIC
			_writes_stat++;
			#endif

			_state.set_to( WRITING );
			btnode_io_completion_handler_write<_Self,SIZE_> handler( this, p_manager );

			_dirty = false;
			request_ptr ret = _p_block->write( _bid, handler );
			return  ret;
		}

	public:

		//! returns BID of the node
		stxxl::BID<SIZE_> bid() const
		{
			//_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			bid_type bid = _bid;
			//_node_mutex.unlock();
			return bid ;
		}

		//! returns BID of the successor of the node
		stxxl::BID<SIZE_> bid_next() const
		{
			STXXL_ASSERT( _p_block );
			//_node_mutex.lock();
			bid_type bid = _p_block->elem->_bid_next;
			//_node_mutex.unlock();
			return bid;
		}

		//! sets BID of the successor of the node
		void bid_next( stxxl::BID<SIZE_> bid )
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			_dirty = true;
			_p_block->elem->_bid_next = bid ;
			_node_mutex.unlock();
		}

		//! returns BID of the predecessor of the node
		stxxl::BID<SIZE_> bid_prev() const
		{
			STXXL_ASSERT( _p_block );
			//_node_mutex.lock();
			bid_type bid = _p_block->elem->_bid_prev ;
			//_node_mutex.unlock();
			return bid;
		}

		//! sets BID of the predecessor of the node
		void bid_prev( stxxl::BID<SIZE_> bid )
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			_dirty = true;
			_p_block->elem->_bid_prev = bid ;
			_node_mutex.unlock();
		}

		// this functions are not thread secure.
		// the client implementation have this to syncron.
		const_reference operator[]( size_type offste ) const { return _p_values[offste];}
		reference 			operator[]( size_type offste )       { return _p_values[offste];}
		value_type*			values()														 { return _p_values; }

		// ----------------------------------------------------------------------------------
		// Searching
		// ----------------------------------------------------------------------------------

		//! find function
		iterator find( const key_type& key )
		{
			iterator ret;

			_node_mutex.lock();

			STXXL_ASSERT( _p_block );
			size_type pos = _find_lower( 0, _p_block->elem->_cur_count, key );
			if ( _p_values[ pos ].first != key )
				ret = end ();
			else
				ret = iterator( this, pos );

			_node_mutex.unlock();

			return ret;
		}

		//! find lower_bound
		iterator lower_bound( const key_type& key )
		{
			_node_mutex.lock();

			STXXL_ASSERT( _p_block );
			size_type s = 0;
			size_type e = _p_block->elem->_cur_count;
			iterator ret = iterator( this, _find_lower( s, e, key ) );

			_node_mutex.unlock();

			return ret;
		}

		//! find less_bound
		iterator less_bound( const key_type& key )
		{
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			size_type s = 0;
			size_type e = _p_block->elem->_cur_count;
			size_type l = _find_lower( s, e, key );
			iterator ret;
			if ( l == 0 ) ret = iterator( this, _p_block->elem->_cur_count);
			else ret = iterator( this, l - 1);

			_node_mutex.unlock();

			return ret;
		}

		//! find lower_bound
		const_iterator lower_bound( const key_type& key ) const
		{
			STXXL_ASSERT( _p_block );
			return const_iterator( this, _find_lower( 0, _p_block->elem->_cur_count, key ) );
		}

		//! find upper_bound
		iterator upper_bound( const key_type& key )
		{
			STXXL_ASSERT( _p_block );
			return iterator( this, _find_upper( 0, _p_block->elem->_cur_count, key ) );
		}

		//! find upper_bound
		const_iterator upper_bound( const key_type& key ) const
		{
			STXXL_ASSERT( _p_block );
			return const_iterator( this, _find_upper( 0, _p_block->elem->_cur_count, key ) );
		}

		//! find equal_range
		std::pair<iterator,iterator> equal_range( const key_type& key )
		{
			STXXL_ASSERT( _p_block );
			size_type pos = _find_lower( 0, _p_block->elem->_cur_count, key );
			if( pos == _p_block->elem->_cur_count || _p_values[pos].first != key )
				return std::pair<iterator,iterator>( end(), end() );
			else
				return std::pair<iterator,iterator>(
					iterator( this, pos ),
					iterator( this, _find_upper( pos, _p_block->elem->_cur_count, key ) ) );
		}

		//! count of the entries
		size_type count( const key_type& key ) const
		{
			STXXL_ASSERT( _p_block );
			return _find_upper( 0, _p_block->elem->_cur_count, key ) - _find_lower( 0, _p_block->elem->_cur_count, key );
		}

		// sizing functions
		bool							empty() const 		{ return _p_block->elem->_cur_count == 0; }

		//! The current number of entries the node contains.
		/*!
		This number can't be greate then max_size of a node.
		So all nodes except the root contain
		at least a half of max_size and maximally max_size entries.
		*/
		size_type					size() const			{ return _p_block->elem->_cur_count; }

		//! The biggest Nummber of entries can be contained by a node.
		/*!
		The max_size of a nodes hangs of the size of the types,
		we use for key and data for leafs and of the size of keys only for internal node.
		*/
		size_type					max_size() const	{ return _max_count; }

		//! The biggest Nummber of entries can be contained by a node.
		/*!
		This is a static function. Is is not like STL's containers.
		The max_size of a nodes hangs of the size of the types,
		we use for key and data for leafs and of the size of keys only for internal node.
		*/
		static size_type	MAX_SIZE()				{ return _max_count; }

		// -------------------------------------------------------------------------------------------------
		// Functions modificate the node.
		// -------------------------------------------------------------------------------------------------

		//! swaps the entries of two nodes.
		void swap( btnode& x )
		{

			// We don't need to call this function in ou map implementation
			STXXL_ASSERT( 0 );

			// *************************************************************
			// swap all members
			// *************************************************************

			block_type *tmp1 = _p_block;
			_p_block = x._p_block;
			x._p_block = tmp1;
   
			BID<SIZE_> tmp2 = _bid;
			_bid = x._bid;
			x._bid = tmp2;

			bool tmp3 = _dirty;
			_dirty = x._dirty;
			x._dirty = tmp3;

			_p_values = (value_type*) _p_block->elem->_Array;
			x._p_values = (value_type*) x._p_block->elem->_Array;
		}

		//! deletes all entries in a node.
		void clear()
		{
			STXXL_ASSERT( _p_block );
			_p_block->elem->_cur_count 		= 0;
			_p_block->elem->_bid_prev = BID<SIZE_>();
			_p_block->elem->_bid_next = BID<SIZE_>();
		}

		//! function will be called from constructor only.
		template <typename InputIterator_>
		void insert( InputIterator_ first, InputIterator_ last )
		{
			STXXL_ASSERT( _p_block );
			if ( _p_block->elem->_cur_count ) // not empty!
			{
				while( first < last && _p_block->elem->_cur_count < _max_count )
					insert( *(first++) );
			}
			else
			{
				STXXL_ASSERT( is_sorted( first, last, value_comp() ) );
				store_type *ptr = (store_type *) _p_values;
				while( first != last && _p_block->elem->_cur_count < _max_count )
					*( ptr + _p_block->elem->_cur_count++ ) = *first++;
			}
		}

		//! This function inserts new enties to a node
		template <typename InputIterator_>
		size_type insert( InputIterator_ first, InputIterator_ last, btnode* p_next )
		{
			// all entries that leaf the node will be copied into other (empty) nodes
			// current node have to be empty and the next one is new ( must be empty )

			STXXL_ASSERT( _p_block->elem->_cur_count );
			STXXL_ASSERT( !p_next->_p_block->elem->_cur_count );

			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( p_next->_p_block );

			// *************************************************************
			// copy all entries to the other node
			// *************************************************************
					store_type*	ptr1 	= ( store_type *) _p_values;
					store_type*	ptr2 	= ( store_type *) p_next->_p_values;

					size_type help_size = _p_block->elem->_cur_count;
					size_type used_size = 0;
					p_next->_p_block->elem->_cur_count = _p_block->elem->_cur_count;
					_p_block->elem->_cur_count = 0;

					while( help_size-- ) *(ptr2++) = *(ptr1++);


		// **************************************************************
		// insert entries from the other node or from iterator
		// **************************************************************

					ptr1 	= ( store_type *) _p_values;
					ptr2 	= ( store_type *) p_next->_p_values;

					while( first < last && _p_block->elem->_cur_count < _max_count )
					{
						if( value_comp()( (*first), *ptr2 ))
						{
							*(ptr1++) = *(first++);
							used_size++;
						}
						else
						{
							*(ptr1++) = *(ptr2++);
							p_next->_p_block->elem->_cur_count--;
						}
						_p_block->elem->_cur_count++;
					}

		// **************************************************************
		// replace the entries in the other node into the begin of node
		// **************************************************************

					ptr1 	= ( store_type *) p_next->_p_values;
					ptr2 	= ( store_type *) p_next->_p_values + used_size;
					help_size = p_next->_p_block->elem->_cur_count;
					while( help_size-- ) *(ptr1++) = *(ptr2++);


			return used_size;
		}

		//! This function inserts new enties to a node
		std::pair<iterator,bool> insert( const value_type& pair )
		{
			size_type pos = _find_lower( 0, _p_block->elem->_cur_count, pair.first );
			if ( unique_ && (pos < _p_block->elem->_cur_count) && (_p_values[pos].first == pair.first) )
				return std::pair<iterator,bool>( iterator( this, pos), false );
			bool ret = _insert( pos, pair ) ;
			return std::pair<iterator,bool>( iterator( this, pos), ret );
		}

		//! This function test if inserting is posible.
		std::pair<iterator,bool> test_insert( const value_type& pair )
		{
			size_type pos = _find_lower( 0, _p_block->elem->_cur_count, pair.first );

			assert( pos <= _max_count );
			if ( unique_ && (pos < _p_block->elem->_cur_count) && (_p_values[pos].first == pair.first) )
				return std::pair<iterator,bool>( iterator( this, pos), false );
			return std::pair<iterator,bool>( iterator( this, pos), true );
		}

		//! This function inserts new enties to a node
		iterator insert( iterator hint, const value_type& pair )
		{
			size_type pos = hint.offset() ;
			if ( _value_compare( pair, *hint ) )
				pos = _find_lower( 0, pos, pair.first );
			else
				pos = _find_lower( pos, _p_block->elem->_cur_count, pair.first );
			if ( unique_ && pos < _p_block->elem->_cur_count && _p_values[pos] == pair )
				return iterator( this, pos);
			if ( _insert( pos, pair )) return iterator( this, pos);
				else return end();
		}

		//! This function deletes entries
		void erase( iterator position )
		{
			size_type pos = position.offset() ;
			_delete( pos, pos + 1 );
		}

		//! This function deletes entries
		size_type erase( const key_type& key )
		{
			STXXL_ASSERT( _p_block );
			size_type pos1 = _find_lower( 0, _p_block->elem->_cur_count, key );
			size_type pos2 = _find_upper( pos1, _p_block->elem->_cur_count, key );

			value_type *ptr = (value_type*) _p_values;
			ptr += pos1;

			if( (*ptr).first != key ) return 0;

			_delete( pos1, pos2 );
			return pos2 - pos1;
		}

		//! This function deletes entries
		void erase( iterator first, iterator last )
		{
			_delete( first.offset(), last.offset() );
		}

		//! This function returns \b true if the node achieved the maximum of the elements.
		bool full() const
		{
			STXXL_ASSERT( _p_block );
			return _p_block->elem->_cur_count == _max_count;
		}

		//! This function returns \b true if the node achieved the half of maximum of the elements.
		bool half() const
		{
			STXXL_ASSERT( _p_block );
			return _p_block->elem->_cur_count <= _max_count / 2;
		}

		//! This function returns \b true if the number of knots is smaller than the maximum number
		bool less_half() const
		{
			STXXL_ASSERT( _p_block );
			return _p_block->elem->_cur_count < _max_count / 2;
		}

		//! splites the node and copies entries into other node.
		void splitt( _Self* p_node );

		//! internal erase function will be called from B-Tree
		void erase_in_tree( iterator& iter, _Self* p_node, int type );

		//! function wich copies all entries of two nodes into one
		size_type merge( _Self& node );

		//! function wich copies some entries from the successor
		key_type steal_right( _Self& node )
		{
			_node_mutex.lock();
			node._node_mutex.lock();

			STXXL_ASSERT( _p_block );
			if( !half() ) dump(std::cout);
			STXXL_ASSERT( half() );

			store_type *ptr1 		= (store_type*) _p_values;
			store_type *ptr2 		= (store_type*) node._p_values;
			store_type *ptr2_end 	= (store_type*) node._p_values;
			ptr2_end += node._p_block->elem->_cur_count ;

			size_type diff =
				( node._p_block->elem->_cur_count + _p_block->elem->_cur_count + 1 ) / 2 - _p_block->elem->_cur_count;
			size_type ret = diff;

			while( diff-- )
			{
				*( ptr1 + _p_block->elem->_cur_count++ ) = *ptr2++;
				node._p_block->elem->_cur_count--;
			}

			ptr1 = (store_type*) node._p_values;
			while( ptr2 != 	ptr2_end )	*ptr1++ = *ptr2++;
			_dirty = true;
			node._dirty = true;

			node._node_mutex.unlock();
			_node_mutex.unlock();

			return ret;
		}

		//! function wich copies some entries from the predecessor
		key_type steal_left( _Self& node )
		{
			node._node_mutex.lock();
			_node_mutex.lock();
			STXXL_ASSERT( _p_block );
			STXXL_ASSERT( half() );

			store_type* ptr1 = (store_type*) _p_values;
			ptr1 += _p_block->elem->_cur_count;

			size_type 	diff =
				( node._p_block->elem->_cur_count + _p_block->elem->_cur_count + 1 ) / 2 - _p_block->elem->_cur_count ;
			size_type 	ret  = diff;

			while( ptr1 != (store_type*)_p_values )
			{
				--ptr1;
				*( ptr1 + diff ) = *ptr1;
			}

			store_type* ptr2 = (store_type*) node._p_values;
			ptr2 += node._p_block->elem->_cur_count - diff ;
			ptr1 = (store_type*) _p_values;

			while( diff-- )
			{
				*( ptr1++ ) = *( ptr2++ );
				_p_block->elem->_cur_count ++;
				node._p_block->elem->_cur_count --;
			}
			_dirty = node._dirty = true;

			node._node_mutex.unlock();
			_node_mutex.unlock();

			return ret;
		}

		//! function replace only the key of an entry
		void replace_key( iterator iter, key_type key )
		{
			_node_mutex.lock();
			store_type*	ptr = (store_type*) _p_values;
			ptr += iter.offset();
			*ptr = store_type( key, (*ptr).second );
			_dirty = true;
			_node_mutex.unlock();
		}

		//! prety print function
		std::ostream& operator <<( std::ostream & _str ) const
		{
			_str << "bid = "    << bid()      << '\n' ;
			_str << "size = "   << size()     << " of " << max_size() << '\n' ;
			_str << "prev = "   << bid_prev() << '\n' ;
			_str << "next = "   << bid_next() << '\n' ;
			_str << "dirty = "  << _dirty     << '\n' ;

			_str << "<" << _p_values[0].first << ":" << _p_values[0].second << "> ... ";
			_str << "<" << _p_values[_p_block->elem->_cur_count - 1].first << ":" << _p_values[_p_block->elem->_cur_count - 1].second << ">" << '\n' ;
			return _str;
		}

		//! prety print function
		void dump( std::ostream & _str ) const
		{
			_str << "bid = "    << bid()      << '\n' ;
			_str << "size = "   << size()     << " of " << max_size() << '\n' ;
			_str << "prev = "   << bid_prev() << '\n' ;
			_str << "next = "   << bid_next() << '\n' ;
			_str << "dirty = "  << _dirty     << '\n' ;

			unsigned i;
			for( i = 0; i < _p_block->elem->_cur_count; i ++ )
				_str << i << "\t <" << _p_values[i].first << ":" << _p_values[i].second << ">\n" ;
			for( ; i < _max_count; i ++ )
				_str << i << "*\t <" << _p_values[i].first << ":" << _p_values[i].second << ">\n" ;
		}

		//! Simple consistency test
		bool valid() const
		{
			return _p_block->elem->_cur_count >= 0 &&
				_p_block->elem->_cur_count <= _max_count &&
				is_sorted( begin(), end(), value_comp() );
		}

		//! Simple consistency test
		bool valid()
		{
			_node_mutex.lock();
			_n_ref++;
			bool ret =
				_p_block->elem->_cur_count >= 0 &&
				_p_block->elem->_cur_count <= _max_count && is_sorted( begin(), end(), value_comp() ) ;

			_n_ref--;
			_node_mutex.unlock();
			return ret;
		}

		friend class btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>;
		friend class const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>;

	}; // class btnode


	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool
	operator==(
	const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x,
	const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{
		return x.operator==( y );
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool
	operator<(
	const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x,
	const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{
		return x.operator<( y );
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator!=( const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{
		return !( x.operator==( y) );
	}	

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator>( const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{
		return y.operator<( x );
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator <= ( const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{
		return !( y.operator < ( x ) );
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator>=( const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{
		return !( x.operator < ( y ));
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	std::ostream& operator <<( std::ostream & _str , btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& _node )
	{
		return _node.operator<<( _str );
	}

	template<typename Key_,typename Data_>
	std::ostream& operator <<( std::ostream & _str, std::pair<const Key_,Data_>& _value )
	{
		_str << "<" << _value.first << " :: " << _value.second << ">" ;
		return  _str ;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::nil( true );


#ifdef STXXL_BTREE_STATISTIC

	template<
		typename Key_,
		typename Data_,
		typename Compare_,
		unsigned SIZE_,
		bool unique_,
		unsigned CACHE_SIZE_>
	bt_counter btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::_temp_node_stat;

	template<
		typename Key_,
		typename Data_,
		typename Compare_,
		unsigned SIZE_,
		bool unique_,
		unsigned CACHE_SIZE_>
	bt_counter btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::_all_node_stat;

	template<
		typename Key_,
		typename Data_,
		typename Compare_,
		unsigned SIZE_,
		bool unique_,
		unsigned CACHE_SIZE_>
	bt_counter btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::_reads_stat;

	template<
		typename Key_,
		typename Data_,
		typename Compare_,
		unsigned SIZE_,
		bool unique_,
		unsigned CACHE_SIZE_>
	bt_counter btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::_writes_stat;

	#endif

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	void btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::splitt(btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>* p_node)
	{
		_node_mutex.lock();
		
		STXXL_ASSERT( _p_block );
		STXXL_ASSERT( full() );

		smart_ptr<_Self> sp_node = p_node;
		smart_ptr<_Self> sp_next_node ;

		// ***********************************************************
		// itit variables
		// ***********************************************************
		size_type diff = _p_block->elem->_cur_count / 2 ;
		_p_block->elem->_cur_count -= diff;
		sp_node->_p_block->elem->_cur_count += diff;

		store_type* ptr1 = (store_type*) _p_values;
		store_type* ptr2 = (store_type*) sp_node->_p_values;
		ptr1 += _p_block->elem->_cur_count;

		// ************************************************************
		// copy the entries
		// ************************************************************
		while( diff-- ) *(ptr2++) = *(ptr1++);

		// move to the btree
		// sp_node->_p_block->elem->_bid_prev = _bid;
		// sp_node->_p_block->elem->_bid_next = _p_block->elem->_bid_next;
  	// _p_block->elem->_bid_next = sp_node->_bid;

		_dirty = true;
		sp_node->_dirty = true;
		_node_mutex.unlock();
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	_SIZE_TYPE btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::merge( btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& node )
	{

		// **********************************************************
		// lock both nodes
		// **********************************************************
		_node_mutex.lock();
		node._node_mutex.lock();

		STXXL_ASSERT( _p_block );
		STXXL_ASSERT( node._p_block );
		STXXL_ASSERT( _p_block->elem->_cur_count + node._p_block->elem->_cur_count <= _max_count );
 
		// ***********************************************************
		// itit variables
		// ***********************************************************
		store_type*	ptr1 = (store_type*) _p_values;
		store_type*	ptr2 = (store_type*) node._p_values;

		size_type	diff = node._p_block->elem->_cur_count;
		size_type	ret  = diff;

		// ************************************************************
		// copy the entries
		// ************************************************************
		while( diff-- ) *( ptr1 + _p_block->elem->_cur_count++ ) = *ptr2++;
		_dirty = true;

		// **********************************************************
		// unlock both nodes
		// **********************************************************
		node._node_mutex.unlock();
		_node_mutex.unlock();
 	
		return ret;
	} 	

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	void btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::erase_in_tree(
		btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::iterator& iter,
		btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>* p_node,
		int type )
	{

		STXXL_ASSERT( _p_block );
		STXXL_ASSERT( iter.bid() == _bid );

		size_type offset;

		switch( type )
		{
			// ***************************************************
			// deleted the entry
			// ***************************************************
			case SIMPLY_ERASED:
				_delete( iter.offset(), iter.offset() + 1 );
				break;

			// ***************************************************
			// merged two nodes and then deleted the entry
			// ***************************************************

			case MERGED_NEXT:
				merge( *p_node );
				_delete( iter.offset(), iter.offset() + 1 );
				break;

			case MERGED_PREV:
				offset = iter.offset();
				iter = p_node->begin();
				iter += offset;
				iter += p_node->merge( *this );
				p_node->_delete( iter.offset(), iter.offset() + 1 );
				_dirty = false;
				break;

			// ********************************************************************
			// copies some entries between two nodes and then deleted the entry
			// ********************************************************************
			case STEALED_PREV:
				iter += steal_left( *p_node );
				_delete( iter.offset(), iter.offset() + 1 );
				break;

			case STEALED_NEXT:
				steal_right( *p_node );
				_delete( iter.offset(), iter.offset() + 1 );
				break;

		}
	}

	template< typename Key_, typename Data_, typename Compare_, unsigned SIZE_, bool unique_, unsigned CACHE_SIZE_>
		_SIZE_TYPE btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::_max_count = ___STXXL_MAP_MAX_COUNT___ ;
//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE


#endif
