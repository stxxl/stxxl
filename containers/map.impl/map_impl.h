/***************************************************************************
                          map_impl.h  -  description
                             -------------------
    begin                : Sam Feb 28 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file map_impl.h
//! \brief This file contains a class map_impl the class for \c stxxl::map implementation.

#ifndef __STXXL_MAP_IMPL_H
#define __STXXL_MAP_IMPL_H

#include "btree.h"
#include "map_queue.h"
#include "map_request.h"
#include "map_iterator.h"

__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{


//! \brief Implementation for async calls to B-Tree.
/*!
	In nearly all functions of this class prepared parameters for a asynchronous call.
	The appropriate are thus produced requests
	and its internal parameter are set.
	These can be processed then by the class \c stxxl::map_internal::btree.
	In this class the LOCKS are created,
	which are to steer accesses.
	Here produced structures are attached into a queue stxxl::map_internal::map_queue and processed successively.
*/
template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
class map_impl : public ref_counter< map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_> >
{
	#ifndef BOOST_MSVC
		__glibcpp_class_requires(Data_, _SGIAssignableConcept)
		__glibcpp_class_requires4(_Compare, bool, Key_, Key_, _BinaryFunctionConcept)
	#endif

private:

	// *********************************************************************************
	// type definitions
	// *********************************************************************************

		typedef map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_> _Self;
		typedef btree<Key_,Data_,Compare_,BlkSize_,true,PrefCacheSize_> _BTreeImpl;

	// *********************************************************************************
	// the posible iterators for map
	// *********************************************************************************

		friend class map_iterator< _Self>;
		friend class const_map_iterator< _Self>;
		friend class optimistic_map_iterator< _Self>;
		friend class optimistic_const_map_iterator< _Self>;


	// **********************************************************************************
	// types for requests and LOCKS
	// **********************************************************************************

		typedef map_request<_Self>   request_type;
		typedef map_lock<_Self>      lock_type;

public:

	// **********************************************************************************
	// types for interenal node cache
	// **********************************************************************************

		typedef typename _BTreeImpl::node_manager_type node_manager_type;
		typedef typename _BTreeImpl::leaf_manager_type leaf_manager_type;

	// **********************************************************************************
	// this type allows us to inserts directly into stxxl::map_internal::btree_queue
	// **********************************************************************************
		typedef typename _BTreeImpl::request_type			optimistic_request_type;
		typedef btrequest_const_find<_BTreeImpl>			request_const_find;

		typedef typename _BTreeImpl::iterator					btree_iterator;
		typedef typename _BTreeImpl::const_iterator		const_btree_iterator;

		typedef optimistic_map_iterator<_Self>        optimistic_iterator;
		typedef optimistic_const_map_iterator<_Self>  optimistic_const_iterator;

		typedef _BTreeImpl														btree_type;

	// ***********************************************************************************
	// the types for STL maps
	// ***********************************************************************************

		typedef typename _BTreeImpl::key_type					key_type;
		typedef typename _BTreeImpl::mapped_type			mapped_type;
		typedef typename _BTreeImpl::value_type				value_type;
		typedef typename _BTreeImpl::key_compare			key_compare;
		typedef typename _BTreeImpl::key_compare			value_compare;
		typedef typename _BTreeImpl::reference				reference;
		typedef typename _BTreeImpl::const_reference	const_reference;
		typedef typename _BTreeImpl::size_type				size_type;
		typedef typename _BTreeImpl::difference_type	difference_type;
		typedef typename _BTreeImpl::pointer					pointer;
		typedef typename _BTreeImpl::const_pointer		const_pointer;

		//typedef typename _BTreeImpl::reverse_iterator				reverse_iterator;
		//typedef typename _BTreeImpl::const_reverse_iterator 	const_reverse_iterator;

		typedef map_iterator<_Self>                   iterator;
		typedef const_map_iterator<_Self>             const_iterator;

	private:

		//! the instance of stxxl::map_internal::map_queue
		static map_queue<_Self> _map_queue;
		//! the current B-Tree
		smart_ptr<_BTreeImpl> _sp_tree;

	public:

		// access to private members
		static map_queue<_Self>* map_queue() { return &_map_queue; };
		_BTreeImpl* tree() { return _sp_tree.get(); }
		const _BTreeImpl* tree() const { return _sp_tree.get(); }

#ifdef STXXL_BTREE_STATISTIC
		static bt_counter _count ;
#endif

	// *******************************************************************************
	// Constructors for map_impl
	// *******************************************************************************

		map_impl()
		{
#ifdef STXXL_BTREE_STATISTIC
			_count++;
#endif
			_sp_tree = new _BTreeImpl ;
		}

		map_impl(const Compare_& comp)
		{
#ifdef STXXL_BTREE_STATISTIC
			_count++;
#endif
			_sp_tree = new _BTreeImpl ( comp ) ;
		}

		map_impl(const map_impl& x)
		{
#ifdef STXXL_BTREE_STATISTIC
			_count++;
#endif
			_sp_tree = new _BTreeImpl ( *x._sp_tree.get() ) ;
		}

		template <typename _InputIterator>
		map_impl( _InputIterator first, _InputIterator last	)
		{
#ifdef STXXL_BTREE_STATISTIC
			_count++;
#endif
			_sp_tree = new _BTreeImpl ( first, last ) ;
		}

		template <typename _InputIterator>
		map_impl( _InputIterator first, _InputIterator last, const Compare_& comp )
		{
#ifdef STXXL_BTREE_STATISTIC
  		_count++;
#endif
			_sp_tree = new _BTreeImpl ( first, last, comp ) ;
		}

		virtual ~map_impl()
		{
#ifdef STXXL_BTREE_STATISTIC
			_count--;
#endif

			// **********************************************************
			// befor we delete the map we have to release all nodes
			// a CLEAR request will be created.
			// **********************************************************
			smart_ptr<request_type> sp_request = new request_type( btree_request_base::CLEAR, this, stxxl::default_map_completion_handler() );
			btrequest_clear<btree_type>* p_param = (btrequest_clear<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );

			map_queue()->add_request( sp_request );
			sp_request->wait();
		}

		static void flush()
		{
			return _BTreeImpl::flush();
		}

		//! Request for the size of map.
		/*!
		We have to lock for all erase and insert operations.
		The request stxxl::map_internal::btrequest_size will be created and added to the map_queue.
		\param p_size contains the right size of map if request complete.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> asize( size_type* p_size, map_completion_handler on_compl = stxxl::default_map_completion_handler() )  const
		{
			STXXL_ASSERT( p_size );
			smart_ptr<request_type> sp_request = new request_type( btree_request_base::SIZE, this, on_compl );
			btrequest_size<btree_type>* p_param = (btrequest_size<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_size = p_size;
			p_param->_p_lock = new map_lock<_Self>((unsigned long) this,  map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

  	//! Request for the max size of map.
		/*!
		\return directly the size.
		*/
		size_type max_size() const
		{
			return _sp_tree->max_size();
		}

		//! Request for the end of map.
		/*!
		\return directly the static object for end. \see stxxl::map_internal::btnode::nil.
		*/
		iterator end() // returns a static object
		{
			btree_iterator *end = new btree_iterator( _sp_tree->end() );
			return iterator( this, end );
		}

		//! Request for the end of map.
		/*!
		\return directly the static object for end. \see stxxl::map_internal::btnode::nil.
		*/
		const_iterator const_end() // returns a static object
		{
			const_btree_iterator *end = new const_btree_iterator( _sp_tree->end() );
			return const_iterator( this, end );
		}

		// Optimistic request for the end of map.
		optimistic_iterator optimistic_end() // returns a static object
		{
			btree_iterator *end = new btree_iterator( _sp_tree->end() );
			return optimistic_iterator( this, end );
		}

		// Optimistic request for the end of map.
		optimistic_const_iterator const_optimistic_end() // returns a static object
		{
			btree_iterator *end = new btree_iterator( _sp_tree->end() );
			return optimistic_iterator( this, end );
		}

		//! Request for the begin of map.
		/*!
		We have to lock for all erase and insert operations at the begin of map.
		The request stxxl::map_internal::btrequest_begin will be created and added to the map_queue.
		\param pp_iter contains the pointer to an iterator to begin of map if request complete.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> abegin( btree_iterator** pp_iter, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::BEGIN, this, on_compl );
			btrequest_begin<btree_type>* p_param = (btrequest_begin<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for the begin of map.
		/*!
		We have to lock for all erase and insert operations at the begin of map.
		The request stxxl::map_internal::btrequest_const_begin will be created and added to the map_queue.
		\param pp_iter contains the pointer to an iterator to begin of map if request complete.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> abegin( const_btree_iterator** pp_iter, map_completion_handler on_compl = stxxl::default_map_completion_handler() ) const
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::CONST_BEGIN, this, on_compl );
			btrequest_const_begin<btree_type>* p_param = (btrequest_const_begin<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_const_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::S );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for the lower_bound of map.
		/*!
		We have to lock for all erase and insert operations at the begin of map.
		The request stxxl::map_internal::btrequest_lower_bound will be created and added to the map_queue.
		\param pp_iter contains the pointer to an iterator to lower_bound of map if request complete.
		\param key the key we are searching.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> alower_bound( btree_iterator** pp_iter, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::LOWER_BOUND, this, on_compl );
			btrequest_lower_bound<btree_type>* p_param = (btrequest_lower_bound<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_OPEN, key, key, map_lock<_Self>::X );
			p_param->_key = key;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for the lower_bound of map.
		/*!
		We have to lock for all erase and insert operations at the begin of map.
		The request stxxl::map_internal::btrequest_const_lower_bound will be created and added to the map_queue.
		\param pp_iter contains the pointer to an const_iterator to lower_bound of map if request complete.
		\param key the key we are searching.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> alower_bound( const_iterator** pp_iter, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() )  const
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::CONST_LOWER_BOUND, this, on_compl );
			btrequest_const_lower_bound<btree_type>* p_param = (btrequest_const_lower_bound<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_const_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_OPEN, key, key, map_lock<_Self>::S );
			p_param->_key = key;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Access to an entry of map.
		/*!
		We call the lower_bound for this key.
		And if there is no entry for this key an entry is added to the map.
		\param key the key we seaching.
		\param on_compl internal using.
		\return the reference to the mapped data.
		*/
		mapped_type&
		operator[](const key_type& key)
		{
			iterator iter = lower_bound( key );
			if ( iter == end() || key_comp()( key, (*iter).first ) )
			{
				value_type value( key, mapped_type() );
				iter = insert( iter, value );
			}
			return (*iter).second;
		}

		//! Request for the find of map.
		/*!
		We have to lock for all erase and insert operations for this key.
		The request stxxl::map_internal::btrequest_find will be created and added to the map_queue.
		\param pp_iter contains the pointer to an iterator to find of map if request complete.
		\param key the key we are searching.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> afind( btree_iterator** pp_iter, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::FIND, this, on_compl );
			btrequest_find<btree_type>* p_param = (btrequest_find<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_CLOSE, key, key, map_lock<_Self>::X );
			p_param->_key = key;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for the find of map.
		/*!
		We have to lock for all erase and insert operations for this key.
		The request stxxl::map_internal::btrequest_const_find will be created and added to the map_queue.
		\param pp_iter contains the pointer to an const_iterator to find of map if request complete.
		\param key the key we are searching.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> afind( const_btree_iterator** pp_iter, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() )  const
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::CONST_FIND, this, on_compl );
			btrequest_const_find<btree_type>* p_param = (btrequest_const_find<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_const_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_CLOSE, key, key, map_lock<_Self>::S );
			p_param->_key = key;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for the upper_bound of map.
		/*!
		We have to lock for all erase and insert operations for this key.
		The request stxxl::map_internal::btrequest_upper_bound will be created and added to the map_queue.
		\param pp_iter contains the pointer to an iterator to upper_bound of map if request complete.
		\param key the key we are searching.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aupper_bound( btree_iterator** pp_iter, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::UPPER_BOUND, this, on_compl );
			btrequest_upper_bound<btree_type>* p_param = (btrequest_upper_bound<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_OPEN, key, key, map_lock<_Self>::X );
			p_param->_key = key;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for the upper_bound of map.
		/*!
		We have to lock for all erase and insert operations for this key.
		The request stxxl::map_internal::btrequest_upper_bound will be created and added to the map_queue.
		\param pp_iter contains the pointer to an const_iterator to upper_bound of map if request complete.
		\param key the key we are searching.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aupper_bound( const_btree_iterator** pp_iter, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() ) const
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::CONST_UPPER_BOUND, this, on_compl );
			btrequest_const_upper_bound<btree_type>* p_param = (btrequest_const_upper_bound<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_const_iterator = pp_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_OPEN, key, key, map_lock<_Self>::S );
			p_param->_key = key;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for inserting into map.
		/*!
		We have to lock for all read, write, erase and insert operations for the whole path to the inserting entry in B-Tree.
		The request stxxl::map_internal::btrequest_insert will be created and added to the map_queue.
		\param pp_iter contains the pointer to an iterator to the entry we inserted or fund if request complete.
		\param p_ret in this pointer the result of inserting will be writen.
				\b true if the entry is inserted or \b false if it  allready in the map.
		\param pair the enty we will insert.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> ainsert( btree_iterator** pp_iter, bool* p_ret, const value_type& pair, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( pp_iter );
			STXXL_ASSERT( !*pp_iter );
			STXXL_ASSERT( p_ret );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::INSERT, this, on_compl );
			btrequest_insert<btree_type>* p_param = (btrequest_insert<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = pp_iter;
			p_param->_p_bool = p_ret;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			p_param->_pair = pair;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for inserting into map.
		/*!
		We have to lock for all read, write, erase and insert operations for the whole path to the inserting entry in B-Tree.
		The request stxxl::map_internal::btrequest_insert will be created and added to the map_queue.
		\param pp_iterator contains the pointer to an iterator to the entry we inserted or fund if request complete.
		\param p_hint pointer to a hint.
		\param pair the enty we will insert.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> ainsert( btree_iterator** pp_iterator, btree_iterator* p_hint, const value_type& pair, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( pp_iterator );
			STXXL_ASSERT( !*pp_iterator );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::INSERT, this, on_compl );
			btrequest_insert<btree_type>* p_param = (btrequest_insert<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = pp_iterator;
			p_param->_p_bool = NULL;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			p_param->_pair = pair;
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for bulk inserting into map.
		/*!
		We have to lock for all read, write, erase and insert operations for whole paths between the insertinng entries.
		The request stxxl::map_internal::btrequest_bulk_insert will be created and added to the map_queue.
		\param first the iterator to the first entry we will insert.
		\param last the iterator to the past last entry we will inserted.
		\param on_compl internal using.
		\return request for this call.
		*/
		template <typename _InputIterator>
		smart_ptr<request_type> ainsert( _InputIterator first, _InputIterator last, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			smart_ptr<request_type> sp_request = new request_type( first, btree_request_base::BULK_INSERT, this, on_compl );
			btrequest_bulk_insert<btree_type,_InputIterator>* p_param = (btrequest_bulk_insert<btree_type,_InputIterator>*) sp_request->_sp_int_request.get();
			p_param->_first = first;
			p_param->_last = last;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for bulk ereasing from the map.
		/*!
		We have to lock for all read, write, erase and insert operations for whole paths between the ereasing entries.
		The request stxxl::map_internal::btrequest_erase_interval will be created and added to the map_queue.
		\param p_first a pointer to iterator to the first entry we will erase.
		\param p_last a pointer to iterator to the past last entry we will erase.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aerase( btree_iterator* p_first, btree_iterator* p_last, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( p_first );
			STXXL_ASSERT( p_last );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::ERASE_INTERVAL, this, on_compl );
			btrequest_erase_interval<btree_type>* p_param = (btrequest_erase_interval<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_first = p_first;
			p_param->_p_last  = p_last;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for ereasing from the map.
		/*!
		We have to lock for all read, write, erase and insert operations for whole paths to the erasing entry.
		The request stxxl::map_internal::btrequest_erase_key will be created and added to the map_queue.
		\param p_size a pointer to size of deleting entries (for map 1 or 0).
		\param key key we want delete.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aerase( size_type* p_size, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( p_size );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::ERASE_KEY, this, on_compl );
			btrequest_erase_key<btree_type>* p_param = (btrequest_erase_key<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_size = p_size;
			p_param->_key = key;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for ereasing from the map.
		/*!
		We have to lock for all read, write, erase and insert operations for whole paths to the erasing entries.
		The request stxxl::map_internal::btrequest_erase will be created and added to the map_queue.
		\param p_iter pointer to the iterator to the entry we will delete.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aerase( btree_iterator* p_iter, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( p_iter );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::ERASE, this, on_compl );
			btrequest_erase<btree_type>* p_param = (btrequest_erase<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_iter = p_iter;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		void clear()
		{
			smart_ptr<request_type> sp_req = aclear();
			sp_req->wait();
		}

		//! Request for ereasing whole map.
		/*!
		We have to lock for all read, write, erase and insert operations for whole map.
		The request stxxl::map_internal::btrequest_clear will be created and added to the map_queue.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aclear( map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			smart_ptr<request_type> sp_request = new request_type( btree_request_base::CLEAR, this, on_compl );
			btrequest_clear<btree_type>* p_param = (btrequest_clear<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		key_compare key_comp() const      { return _sp_tree->key_comp(); }
		value_compare value_comp() const  { return _sp_tree->value_comp(); }

		//! Request for count for a key.
		/*!
		We have to lock for all read, write, erase and insert operations for whole map.
		The request stxxl::map_internal::btrequest_count will be created and added to the map_queue.
		\param p_size contains the number of entries for the key if request complete.
		\param key we are searching for.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> acount( size_type* p_size, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			STXXL_ASSERT( p_size );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::COUNT, this, on_compl );
			btrequest_count<btree_type>* p_param = (btrequest_count<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_size = p_size;
			p_param->_key = key;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_CLOSE, key, key, map_lock<_Self>::S );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for begin and end iterators for range of a key.
		/*!
		We have to lock for all read, write, erase and insert operations for whole map.
		The request stxxl::map_internal::btrequest_equal_range will be created and added to the map_queue.
		\param pp_iter1 contains the pointer to an iterator to begin of range if request complete.
		\param pp_iter2 contains the pointer to an iterator to end of range if request complete.
		\param key we are searching for.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aequal_range( btree_iterator **pp_iter1, btree_iterator** pp_iter2, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler())
		{
			STXXL_ASSERT( pp_iter1 );
			STXXL_ASSERT( !*pp_iter1 );
			STXXL_ASSERT( pp_iter2 );
			STXXL_ASSERT( !*pp_iter2 );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::EQUAL_RANGE, this, on_compl );
			btrequest_equal_range<btree_type>* p_param = (btrequest_equal_range<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_lower_iterator = pp_iter1;
			p_param->_pp_upper_iterator = pp_iter2;
			p_param->_key = key;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_CLOSE, key, key, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		//! Request for begin and end const iterators for range of a key.
		/*!
		We have to lock for all read, write, erase and insert operations for whole map.
		The request stxxl::map_internal::btrequest_const_equal_range will be created and added to the map_queue.
		\param pp_iter1 contains the pointer to an iterator to begin of range if request complete.
		\param pp_iter2 contains the pointer to an iterator to end of range if request complete.
		\param key we are searching for.
		\param on_compl internal using.
		\return request for this call.
		*/
		smart_ptr<request_type> aequal_range( const_btree_iterator **pp_iter1, const_btree_iterator** pp_iter2, const key_type& key, map_completion_handler on_compl = stxxl::default_map_completion_handler()) const
		{
			STXXL_ASSERT( pp_iter1 );
			STXXL_ASSERT( !*pp_iter1 );
			STXXL_ASSERT( pp_iter2 );
			STXXL_ASSERT( !*pp_iter2 );

			smart_ptr<request_type> sp_request = new request_type( btree_request_base::CONST_EQUAL_RANGE, this, on_compl );
			btrequest_const_equal_range<btree_type>* p_param = (btrequest_const_equal_range<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_const_lower_iterator = pp_iter1;
			p_param->_pp_const_upper_iterator = pp_iter2;
			p_param->_key = key;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::CLOSE_CLOSE, key, key, map_lock<_Self>::S );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		// ******************************************************************************
		// helpfull functions
		// ******************************************************************************

		bool valid()
		{
			return _B_tree.valid();
		}

		// print the whole map.
		smart_ptr<request_type> dump( map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			smart_ptr<request_type> sp_request = new request_type( btree_request_base::DUMP, this, on_compl );
			btrequest_dump<btree_type>* p_param = (btrequest_dump<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) this, map_lock<_Self>::OPEN_OPEN, 0, 0, map_lock<_Self>::X );
			map_queue()->add_request( sp_request );
			return sp_request;
		}

		static void dump_managers()
		{
			btree_type::dump_managers();
		}
	};

#ifdef STXXL_BTREE_STATISTIC

	template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
	bt_counter map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>::_count(0);

#endif

	template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
	map_queue<map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_> > map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>::_map_queue;

	//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE


#endif
