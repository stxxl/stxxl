/***************************************************************************
                          map_async.h  -  description
                             -------------------
    begin                : Sam Feb 28 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file map_async.h
//! \brief This file contains a class map the public interface for \c stxxl::map implementation.


#ifndef __STXXL_MAP_SYNC_H
#define __STXXL_MAP_SYNC_H

#include "smart_ptr.h"
#include "map_impl.h"

namespace stxxl
{

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
class map;

//! \addtogroup stlcontinternals
//! \{

	//! \brief Implementing size_type of stxxl::map container.
	/*!
		This class implements size_type.
		It will be returned by some functions of stxxl::map interface.
		This implamenting binds a type for size value
		and a requests functionality of asynchronous calls.
		\see stxxl::map_iterator and stxxl::const_map_iterator
		*/
	template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
	class map_size_impl : public map_pram_interface
	{

		// *******************************************************************
		// typedef
		// *******************************************************************

			typedef map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>   _Impl;
			typedef map_request<_Impl>                                      request_type;
			typedef smart_ptr<request_type>                                 request_ptr;
			typedef typename _Impl::size_type                               size_type;

		// *******************************************************************
		// members
		// *******************************************************************

			size_type       _size;
			request_ptr     _sp_request;


		// *******************************************************************
		// friends
		// *******************************************************************

			friend class map<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>;

public:

		//! \brief A standard constructor. (no request member will be set )
		map_size_impl( size_type size = 0 ) : _size( size ), _sp_request(NULL) {}

		//! \brief A constructor within the request member will be set.
		map_size_impl( request_ptr sp_request ) : _size(0), _sp_request( sp_request ) {}

		//! \brief Virtual destructor
		virtual ~map_size_impl() {}

		//! \brief Allow us the access to the size value.
		/*!
		 	\return the value for size.

		  This function waits until the size is valid.
		  Blocks following operations.
		*/
		operator size_type() const
		{
			if( _sp_request.ptr ) _sp_request->wait();
			return _size;
		}

		//! \brief The wait function of request member will be called.
		/*!
		We can call this function also directly.
		In this case, the execution of the following operations waits, until this operation is terminated. \n
		That can be crucial in this case, if continue the program depends on the value of the size.
		But in this case we can access simply to value.
		Because in this case the function wait is called internally. \n
		If we needn't the value of size directly,
		we can use poll function direct.
	*/

		virtual void wait()
		{

			// ****************************************************************
			// We have to wait only if the query is processed.
			// ****************************************************************

			if( _sp_request.ptr ) _sp_request->wait();
		}

		//! \brief The poll function of request member will be called.
		/*!
		In this case,
		the execution of the following operations doesn't wait,
		until this operation is terminated. \n
		That can be crucial in this case,
		if we don't need the value of size directly.
		*/
		virtual bool poll()
		{

			// ****************************************************************
			// We have to poll only if the query is processed.
			// In other case we can return true directly.
			// ****************************************************************

			if( _sp_request.ptr ) return _sp_request->poll();
			else return true;
		}
	};

	template <typename Key_>
	class operator_less: public std::less<Key_>
	{
		Key_ min_value() const { return (std::numeric_limits<Key_>::min)(); };
		Key_ max_value() const { return (std::numeric_limits<Key_>::max)(); };
	};
	//! \}
	
	//! \addtogroup stlcont
	//!
	//! \{

	//! \brief External map container with support of asynchronous access.
	/*! For semantics of the methods see documentation of the STL std::map.\n
		Template parameters:
		- \c Key_ The map's key type
		- \c Data_ The map's data type
		- \c Compare_ The map's key comparsion function
		- \c BlkSize_ external block size in bytes, default is 4096 kbytes
		- \c PrefCacheSize_ the prefere internal cache size = the nummber of blocks we will hold in it, default is 16
	*/
	template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_ = 4096, unsigned PrefCacheSize_= 16>
	class map
	{

	// ***************************************************************************
	// typedef
	// ***************************************************************************

	private:

		typedef map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_> 	_Impl;
		typedef map<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>				_Self;
		typedef map_request_base																	request_base;
		typedef map_request<_Impl>																request_type;
		typedef map_lock<_Impl>																		lock_type;

		//typedef typename _Impl::request_const_find					request_const_find;
		//typedef typename _Impl::optimistic_request_type			optimistic_request_type;

	public:

		typedef typename _Impl::node_manager_type node_manager_type;
		typedef typename _Impl::leaf_manager_type leaf_manager_type;

		//! A class for size requests
		typedef map_size_impl<Key_,Data_,Compare_,BlkSize_, PrefCacheSize_> size_type;
    //!
		//! Key data type of map
		typedef typename _Impl::key_type                    key_type;
    //!
		//! \brief Data data type of map
		typedef typename _Impl::mapped_type                 mapped_type;
    //!
		//! \brief A type for a pair of a key and data represents an entry in map
		typedef typename _Impl::value_type                  value_type;
    //!
		//! \brief A compare type for two keys
		typedef typename _Impl::key_compare                 key_compare;
    //!
		//! \brief A compare type for two pairs of key and data
		typedef typename _Impl::key_compare                 value_compare;
    //!
		//! \brief A reference type to en entry of map
		typedef typename _Impl::reference                   reference;
    //!
		//! \brief A reference type to an entry of map
		typedef typename _Impl::const_reference             const_reference;
    //!
		//! \brief A type for a iterator diference
		typedef typename _Impl::difference_type             difference_type;
    //!
		//! \brief A pointer to an entry of map
		typedef typename _Impl::pointer                     pointer;
    //!
		//! \brief A pointer to an entry of map
		typedef typename _Impl::const_pointer               const_pointer;
    //!
		//typedef typename _BTreeImpl::reverse_iterator         reverse_iterator;
		//typedef typename _BTreeImpl::const_reverse_iterator   const_reverse_iterator;
    //!
		//! \brief A type for an iterator
		typedef typename _Impl::iterator                    iterator;
    //!
		//! \brief A type for an iterator
		typedef typename _Impl::const_iterator              const_iterator;

		// *******************************************************************************
		// iterator can be used to access withaut check the map lock.
		// *******************************************************************************
		typedef typename _Impl::optimistic_iterator         optimistic_iterator;
		typedef typename _Impl::optimistic_const_iterator   optimistic_const_iterator;


		// *******************************************************************************
		// usefull smart pointers
		// *******************************************************************************
		typedef smart_ptr<iterator>          iterator_ptr;
		typedef smart_ptr<const_iterator>    const_iterator_ptr;
		typedef smart_ptr<request_type>      request_ptr;

	private:

		// *******************************************************************************
		// this is the class contains the implementation of map.
		// *******************************************************************************

		smart_ptr<_Impl>  _sp_map;

		// *******************************************************************************
		// Constructors for map
		// *******************************************************************************

public:

		//! \brief A standard constructor for a map
		map()
		{
			// Only a new implementation object will be created.
			_sp_map = new _Impl ;
		}

		//! \brief A constructor for a map, where the special class for _Compare will be used.
		map(const Compare_& comp)
		{
			// Only a new implementation object will be created.
			_sp_map = new _Impl( comp ) ;
		}

		//! \brief A copy constructor for a map.
		map(const map& x)
		{
			// Only a new implementation object will be created a copy of an other implementation object.
			_sp_map = new _Impl( *x._sp_map.get() ) ;
		}

		//! \brief A bulk constructor for a map.
		/*!
		A B-Tree will be create bottom up.
		A stxxl::vactor container will be used for the contruction of internal nodes.
		*/
		template <typename _InputIterator>
		map( _InputIterator first, _InputIterator last	)
		{
			// A new implementation object will be created and a vlues will be inserted.
			_sp_map = new _Impl( first, last ) ;
		}

		//! \brief A other bulk constructor for a map. A special object for Compare_ is given.
		/*!
		A B-Tree will be create bottom up.
		A stxxl::vactor container will be used for the contruction of internal nodes.
		*/
		template <typename _InputIterator>
		map( _InputIterator first, _InputIterator last, const Compare_& comp )
		{
			// A new implementation object will be created and a vlues will be inserted.
			_sp_map = new _Impl( first, last, comp ) ;
		}

		//! \brief A destructor for a map
		/*!
			We release only our reference to the internal object.
			The internal object will be destoyd after all references are released.
			It is posible to destroy this object without all requests are handled.
		*/
		virtual ~map()
		{
			// We release the reference.
			// The internal object will be destoyd after all references are released.
			_sp_map.release();
		}

		//! \brief Retruns is the map is empty.
		/*!
		This Function blocks because size have to be valid.
		*/
		bool empty() const
		{
			size_type size;
			size._sp_request = ( _sp_map->asize( &size._size ) );
			return size == 0;
		}

		//! \brief Return the current size of map.
		/*!
		This call don't wait for the internal value of size_type will be valid.
		Our implementation hold the nummber of all nodes
		and decreses or increses it by inserting or deleting an entry.
		\see btnode::size and btree::size.
		*/
		size_type size() const
		{
			size_type size;
			size._sp_request = ( _sp_map->asize( &size._size ) );
			return size;
		}

		//! \brief Return the max_size of the map.
		/*!
		This hangs of the size of posible diferent keys and the size of free memory place.
		*/
		size_type max_size() const
		{
			return 0xFFFFFFFF ;
		}

		//! \brief Returns an iterator to the end of map.
		/*!
		The end of the map is an iterator, shows to a special node stxxl::map_internal::btnode::nil.
		So we don't need to find the end we can gives it directly.
		*/
		iterator end()
		{
			return _sp_map->end();
		}

		//! \brief Returns an const_iterator to the end of map (const case).
		/*!
		The end of the map is an iterator, shows to a special node stxxl::map_internal::btnode::nil.
		So we don't need to find the end we can gives it directly.
		*/
		const_iterator end() const
		{
			return _sp_map->end();
		}

		//! \brief Returns an iterator for begin of the map
		/*!
		We don't have directly reference to the first entry in the \c stxxl::map.
		We must call the function abegin of stxxl::map_internal::map_impl.
		*/
		iterator begin()
		{
			iterator iter( _sp_map.ptr );
			iter._sp_request = _sp_map->abegin( &iter._sp_iterator.ptr );
			return iter;
		}

		//! \brief Returns an const_iterator for begin of the map (const case).
		/*!
		We don't have directly reference to the first entry in the \c stxxl::map.
		We must call the function abegin of stxxl::map_internal::map_impl.
		*/
		const_iterator begin() const
		{
			iterator iter( _sp_map.ptr );
			iter._sp_request = _sp_map->abegin( &iter._sp_iterator.ptr );
			return iter;
		}

		//! \brief Returns an iterator to the entry is not lower the key
		/*!
		Searching for lower bound starts.
		*/
		iterator lower_bound( const key_type& key )
		{
			iterator iter( _sp_map.ptr );
			iter._sp_request = _sp_map->alower_bound( &iter._sp_iterator.ptr, key );
			return iter;
		}

		//! \brief Returns an iterator to the entry is not lower the key
		const_iterator lower_bound( const key_type& key ) const
		{
			iterator iter( _sp_map.ptr );
			iter._sp_request = _sp_map->alower_bound( &iter._sp_iterator.ptr, key );
			return iter;
		}

	//! \brief Direct access to the data by a key.
	mapped_type& operator[](const key_type& key)
	{
		iterator iter( _sp_map.ptr );
		iter._sp_request = _sp_map->alower_bound( &iter._sp_iterator.ptr, key );
		iter._sp_request->wait();

		if ( iter == end() )
		{
			value_type value( key, mapped_type() );
			iterator iter2( _sp_map.ptr );
			iter2._sp_request = _sp_map->ainsert( &iter2._sp_iterator.ptr, iter._sp_iterator.ptr, value );
			iter2._sp_request->wait();
			return (*iter2).second;
		}
		else if( key_comp()( key, (*iter).first ) )
		{
			value_type value( key, mapped_type() );
			iterator iter2( _sp_map.ptr );
			iter2._sp_request = _sp_map->ainsert( &iter2._sp_iterator.ptr, iter._sp_iterator.ptr, value );
			iter2._sp_request->wait();
			return (*iter2).second;
		}
		return (*iter).second;
	}

	//! \brief Returns an iterator to the entry for the key
	iterator find( const key_type& key )
	{
		iterator iter( _sp_map.ptr );
		iter._sp_request = _sp_map->afind( &iter._sp_iterator.ptr, key );
		return iter;
	}

	//! \brief Returns an iterator to the entry for the key
	const_iterator find( const key_type& key ) const
	{
		const_iterator iter( _sp_map.ptr );
		iter._sp_request = _sp_map->afind( &iter._sp_const_iterator.ptr, key );
		return iter;
	}

	//! \brief Returns an iterator to the entry is bigger then the key
	iterator upper_bound( const key_type& key )
	{
		iterator iter( _sp_map.ptr );
		iter._sp_request = _sp_map->aupper_bound( &iter._sp_iterator.ptr, key );
		return iter;
	}

	//! \brief Returns an iterator to the entry is bigger then the key
	const_iterator upper_bound( const key_type& key ) const
	{
		const_iterator iter( _sp_map.ptr );
		iter._sp_request = _sp_map->aupper_bound( &iter._sp_const_iterator.ptr, key );
		return iter;
	}

	//! \brief The size of the map.
	size_type count( const key_type& key ) const
	{
		return _sp_map->acount( &size, key, on_compl );
	}

	//! \brief Returns the first and the last iterator to the entry for the key.
	std::pair<iterator,iterator> equal_range(const key_type& key)
	{
		std::pair<iterator,iterator> ret( _sp_map.ptr, _sp_map.ptr );
//		ret.first._sp_map = ret.second._sp_map = _sp_tree;
		ret.first._sp_request = _sp_map->aequal_range( &ret.first._sp_iterator.ptr, &ret.second._sp_iterator.ptr, key );
		ret.second._sp_request = ret.first._sp_request;
		return ret;
	}

  //! \brief Returns the first and the last iterator to the entry for the key.
  std::pair<const_iterator,const_iterator> equal_range(const key_type& key) const
	{
		std::pair<const_iterator,const_iterator> ret(
			const_iterator( _sp_map.ptr ),
			const_iterator( _sp_map.ptr ));
		ret.first._sp_request = _sp_map->aequal_range(
			&ret.first._sp_const_iterator.ptr,
			&ret.second._sp_const_iterator.ptr,
			key );
		ret.second._sp_request = ret.first._sp_request;
		return ret;
	}

	//! \brief Inserts a value_type in to the map.
	std::pair<iterator,bool> insert( const value_type& pair )
	{
		iterator iter( _sp_map.ptr );
		bool ret = false;
		iter._sp_request = _sp_map->ainsert( &iter._sp_iterator.ptr, &ret, pair );
		iter._sp_request->wait();
		return std::pair<iterator,bool>(iter,ret);
	}

	//! \brief Inserts a value_type in to the map by a hint.
	iterator insert( iterator hint, const value_type& pair )
	{
		iterator iter( _sp_map.ptr );
		iter._sp_request = _sp_map->ainsert( &iter._sp_iterator.ptr, hint._sp_iterator.ptr, pair );
		return iter;
	}

	//! \brief Inserts a sequence of object of type value_type in to the map.
	template <typename _InputIterator>
	void insert( _InputIterator first, _InputIterator last )
	{
		request_ptr sp_request = _sp_map->ainsert( first, last );
		sp_request->wait();
	}

	//! \brief Deletes a sequence of values between two iterators.
	void erase( iterator first, iterator last )
	{
		if ( first._sp_request.ptr ) first._sp_request->wait();
		if ( last._sp_request.ptr ) last._sp_request->wait();
		request_ptr sp_request = _sp_map->aerase( first._sp_iterator.ptr, last._sp_iterator.ptr );
		sp_request->wait();
	}

	//! \brief Deletes all values for a key.
	size_type erase( const key_type& key )
	{
		size_type size;
		size._sp_request = _sp_map->aerase( &size._size, key );
		return size;
	}

	//! \brief Deletes an entry for an iterator.
	void erase( iterator position )
	{
		if ( position._sp_request.ptr ) position._sp_request->wait();
		request_ptr sp_request = _sp_map->aerase( position._sp_iterator.ptr );
		sp_request->wait();
	}

	//! \brief Delete all entries in the map.
	void clear()
	{
		request_ptr sp_request = _sp_map->aclear();
		sp_request->wait();
	}

	//! \brief Prints the whole B-Tree.
	void dump()
	{
		stxxl::default_map_completion_handler h;
		request_ptr sp_request = _sp_map->dump( h);
		sp_request->wait();
	}

	//! \brief Gives the key comparator.
	key_compare key_comp() const      { return _sp_map->key_comp(); }

	//! \brief Gives the value comparator.
	value_compare value_comp() const  { return _sp_map->value_comp(); }
};

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
inline bool operator==( const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& x, const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& y )
{ return x._B_tree == y._B_tree; }

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
inline bool operator<( const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& x, const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& y )
{ return x._B_tree < y._B_tree; }

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
inline bool operator!=( const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& x, const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& y )
{ return !( x == y); }

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
inline bool operator>( const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& x, const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& y )
{ return y < x; }

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
inline bool operator<=( const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& x, const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& y )
{ return !( y < x ); }

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
inline bool operator>=( const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& x, const map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& y )
{ return !( x < y); }

template <typename Key_, typename Data_, typename Compare_, unsigned BlkSize_, unsigned PrefCacheSize_>
inline void swap( map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& x, map_impl<Key_,Data_,Compare_,BlkSize_,PrefCacheSize_>& y )
{ x.swap( y ); }

  //! \}
  
}

#endif
