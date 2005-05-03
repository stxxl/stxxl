/***************************************************************************
                          btnode_manager.h  -  description
                             -------------------
    begin                : Fre Apr 30 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file btnode_manager.h
//! \brief Implemets the struct for caching and managing of \c stxxl::map_internal::btnodes for the \c stxxl::map container.

#include "map.h"
#include "btree_request.h"
#include "bt_counter.h"


#ifndef __STXXL_BTNODE_MENAGER_H
#define __STXXL_BTNODE_MENAGER_H

__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	//! \brief An interface to remove a reference to a node,
	//! we don't need in the local node cache manager.
	/*!
	We need a function after we have the node write in to the extern memory,
	to remove this node from the local cache.

	This function will call by btnode_io_completion_handler_write.
	If we needn't to write a node back to the disk,
	we will this function directly from a node.
	\see node::write
	*/
	template<unsigned SIZE_>
	class async_btnode_manager_base
	{
		public:
			//! Remove a referenc to the node from the local cache manager.
			/*!
			\param bid
			*/
			virtual void remove( const stxxl::BID<SIZE_>&  bid ) = 0;
	};

	//! \brief The node of a certain type are managed here.
	/*!
	Hier kommt mehr Text ..
	*/
	template< typename Node, typename Btree, unsigned SIZE_, unsigned CACHE_SIZE_>
	class async_btnode_manager : public async_btnode_manager_base<SIZE_>
	{

		// *****************************************************************
		// typedefs
		// *****************************************************************

			typedef async_btnode_manager<Node,Btree,SIZE_,CACHE_SIZE_>   _Self;

			// this is a member can be used for debuging.
  		// typedef std::map< bid_type,bool>              bool_map_type ;

			typedef smart_ptr<Node>                          node_ptr;
			typedef typename Btree::bid_type                 bid_type;
			typedef std::map< bid_type,node_ptr>             map_type ;
			typedef typename Btree::request_type             btree_request_type;
			typedef typename map_type::value_type            map_value_type ;
			typedef typename map_type::iterator              map_iterator_type ;
			typedef typename map_type::const_iterator        map_const_iterator_type ;
			typedef typename map_type::size_type             map_size_type;

		// *****************************************************************
		// members
    // *****************************************************************

		private:

			static _Self* _p_instance ;				// a instance of async_btnode_manager

			#ifdef STXXL_BOOST_THREADS
			boost::mutex _mutex; // a member for synchronize access
			#else
			mutex 				_mutex;							// a member for synchronize access
			#endif
			map_type 			_internal;					// this is a container for nodes
			block_manager* _p_block_manager;	// a pointer to the block_manager

			// this is a member can be used for debuging.
			// bool_map_type _all;

		public:

			bt_counter _count;



		// ******************************************************************
		// methodes
		// ******************************************************************

		public:

			async_btnode_manager() : _p_block_manager( block_manager::get_instance() ), _count( 0 ) {};

			virtual ~async_btnode_manager()
			{
				//! \todo Problemms?

				/*
				_mutex.lock();
				if ( _internal.size() > CACHE_SIZE_ )
				{
					map_iterator_type iter = _internal.begin();
					while( iter != _internal.end() )
					{
						_internal.erase( iter );
						iter = _internal.begin();
					}
					// this is a member can be used for debuging.
					// _all.clear();
				}
				_mutex.unlock();
				*/
			}

			static _Self* get_instance()
			{
				if( !_p_instance ) _p_instance = new _Self();
				return _p_instance;
			}

			//! Call this function to remove the node from local cache.
			virtual void remove( const stxxl::BID<SIZE_>&  bid )
			{
				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex);
				#else
				_mutex.lock();
				#endif
				// this is a member can be used for debuging.
				// assert( _all.find( bid ) != _all.end() );
				_internal.erase( _internal.find( bid ) );
				#ifdef STXXL_BOOST_THREADS
				Lock.unlock();
				#else
				_mutex.unlock();
				#endif
				--_count;
			}

      //void GetNewNode( InputIterator_ first, InputIterator_ last, Node** ppNode )
			
			//! Call this function to remove the node from local cache.
			/*! \see async_btnode_manager_base
			*/
			template <typename InputIterator_>
			void GetNewNode( InputIterator_ first, InputIterator_ last, Node** ppNode )
			{
				cleen();

				STXXL_ASSERT( ppNode );
				STXXL_ASSERT( !*ppNode );

				*ppNode = new Node( first, last );
				(*ppNode)->allocate_block( _p_block_manager );
				(*ppNode)->add_ref();

				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex);
				#else
				_mutex.lock();
				#endif
				_internal.insert( map_value_type( (*ppNode)->bid(), node_ptr(*ppNode) ));
				// this is a member can be used for debuging.
  			// _all.insert( bool_map_type::value_type( (*ppNode)->bid(), true ) );
				#ifndef STXXL_BOOST_THREADS
				_mutex.unlock();
				#endif
			}

			//! A new node will be created.
			/*!
			\param ppNode a refernce to pointer, the new node can be created.
			This function will be call if a new external memory node is needed.
			A new block will be allocated and writen to the local cache.
			*/
			void GetNewNode( Node** ppNode )
			{
				STXXL_ASSERT( ppNode );
				STXXL_ASSERT( !*ppNode );

				cleen();

				_count.wait_for(0);

				// ************************************************************************
				// create new node and allocate a block in external memory
				// ************************************************************************

				*ppNode = new Node;
				(*ppNode)->allocate_block( _p_block_manager );
				(*ppNode)->add_ref();


				// ************************************************************************
				// insert the new node into lacal cache
				// ************************************************************************
		
				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex);
				#else
				_mutex.lock();
				#endif
				_internal.insert( map_value_type( (*ppNode)->bid(), node_ptr(*ppNode) ));

				// this is a member can be used for debuging.
  			// _all.insert( bool_map_type::value_type( (*ppNode)->bid(), true ) );

				#ifndef STXXL_BOOST_THREADS
				_mutex.unlock();
				#endif
			}

			//! Returns a node for a BID.
			/*!
			This function will be call if a new external memory node is needed.
			A new block will be allocated and writen to the local cache.
			*/
			request_ptr GetNode( bid_type bid, Node** ppNode, btree_request_type* p_btrequest )
			{
				STXXL_ASSERT( bid.valid() );
				STXXL_ASSERT( ppNode );
				STXXL_ASSERT( !*ppNode );
				STXXL_ASSERT( p_btrequest );

				cleen();
				_count.wait_for(0);

				request_ptr ret;
				node_ptr sp_node;

				// *******************************************************************
				// search if already loaded
				// *******************************************************************

				//STXXL_MSG( "SEARCH start" << bid );
				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex);
				#else
				_mutex.lock();
				#endif

				// this is a member can be used for debuging.
				// assert( _all.find( bid ) != _all.end() );

				map_iterator_type iter = _internal.find( bid );
				if ( iter != _internal.end() )
				{
					sp_node = (*iter).second;
					#ifdef STXXL_BOOST_THREADS
					Lock.unlock();
					#else
					_mutex.unlock();
					#endif
					*ppNode = sp_node.get();
					sp_node->add_ref();
					STXXL_ASSERT( (*iter).first == (*ppNode)->bid() );
					return sp_node->add_request( p_btrequest );
				}

				// *******************************************************************
				// node can't found
				// create new Entry
				// *******************************************************************

				*ppNode = new Node( bid );
				_internal.insert( map_value_type( bid, node_ptr( *ppNode ) ));

				#ifdef STXXL_BOOST_THREADS
				Lock.unlock();
				#else
				_mutex.unlock();
				#endif
				sp_node = *ppNode;
				sp_node->add_ref();
				sp_node->read();
				return sp_node->add_request( p_btrequest );
			}

			//! The block in external memory becommes free.
			/*!
			We release the external memory space and remove the reference to the local cache.
			*/
			void ReleaseNode( bid_type bid )
			{
				_count.wait_for(0);
    
				STXXL_ASSERT( bid.valid() );
				request_ptr ptr;
				node_ptr sp_node;

				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex);
				#else
				_mutex.lock();
				#endif

				//! \todo this is a member can be used for debuging.
  			// assert( _all.find( bid ) != _all.end() );
				// _all.erase( bid );

				map_iterator_type iter = _internal.find( bid );
				if( iter != _internal.end() )
				{
					sp_node = (*iter).second;
					_internal.erase( iter );
				}
				#ifdef STXXL_BOOST_THREADS
				Lock.unlock();
				#else
				_mutex.unlock();
				#endif

				if( sp_node.get() ) sp_node->set_dirty( false );
				Node::free_block( bid, _p_block_manager );
			}

		void dump()
		{
			smart_ptr<Node> sp_node;
			_count.wait_for(0);
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(_mutex);
			#else
			_mutex.lock();
			#endif

			//! @todo remove this
			STXXL_MSG( "DUMP SIZE = " << _all.size() );
			int count = 0;

			//! \todo this is a member can be used for debuging.
  		// typename bool_map_type::iterator iter = _all.begin();
			// while( iter != _all.end() )
			// STXXL_VERBOSE2( count++ << ": " << (iter++)->first );

			#ifndef STXXL_BOOST_THREADS
			_mutex.unlock();
			#endif
		}

		void cleen()
		{
			smart_ptr<Node> sp_node;
			_count.wait_for(0);
   
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(_mutex);
			#else
			_mutex.lock();
			#endif
			map_iterator_type iter = _internal.begin();

			for(;;)
			{
  
				if ( CACHE_SIZE_ > _internal.size() )
				{
					#ifndef STXXL_BOOST_THREADS
					_mutex.unlock();
					#endif
					return;
				}

				if( iter == _internal.end() )
				{
					#ifndef STXXL_BOOST_THREADS
					_mutex.unlock();
					#endif
					return;
				}

				if( (*iter).second->get_count() == 1 )
				{
					++_count;
					sp_node = (*iter).second;
				}

				#ifdef STXXL_BOOST_THREADS
				Lock.unlock();
				#else
				_mutex.unlock();
				#endif
   
				if ( sp_node.get() )
				{
					sp_node->write( this );
					sp_node.release();
				}

				#ifdef STXXL_BOOST_THREADS
				Lock.lock();
				#else
				_mutex.lock();
				#endif
				++iter;
			}
		}
	}; // class btnode_menager


	template< typename Node, typename Btree, unsigned SIZE_, unsigned CACHE_SIZE_>
	async_btnode_manager<Node,Btree,SIZE_,CACHE_SIZE_>* async_btnode_manager<Node,Btree,SIZE_,CACHE_SIZE_>::_p_instance = NULL;

//! \}
} // namespace map_internal

__STXXL_END_NAMESPACE


#endif
