/***************************************************************************
                          map_iterator.h  -  description
                             -------------------
    begin                : Don Okt 7 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file map_iterator.h
//! \brief This file contains the iterator classes for \c stxxl::map implementation.
  
#include "btree.h"
#include "map_queue.h"
#include "map_request.h"

namespace stxxl
{
	typedef default_completion_handler default_bree_completion_handler;

	using namespace map_internal;

	//! Interface for return types for asycronous functions of \c stxxl::map container.
	class map_pram_interface
	{
	public:
		virtual void wait() = 0;
		virtual bool poll() = 0;
	};

	//! \brief The iterator for \c stxxl::map container.
	/*!
		This class implements an iterator type.
		It will be returned by some functions of stxxl::map interface.
		This implamenting binds a type for iterator concept
		and a requests functionality of asynchronous calls.
		\see stxxl::map_size_impl and stxxl::const_map_iterator
	*/
	template<typename Map>
	class map_iterator : public map_pram_interface // : public ref_counter< map_iterator<Map> >
	{

		// ***************************************************************************
		// typedef
		// ***************************************************************************

	private:

		typedef map_iterator<Map>	_Self;
		typedef Map								map_type;
    
	public:
	
		//! \todo make it private
		typedef 		typename Map::node_manager_type  node_manager_type;
		typedef 		typename Map::leaf_manager_type  leaf_manager_type;
		typedef			typename Map::_BTreeImpl				 btree_type;

		typedef			typename btree_type::iterator		 iterator;
		typedef 		typename Map::reference  	  		 reference;
		typedef 		typename Map::const_reference  	 const_reference;
		typedef 		typename Map::pointer						 pointer;
		typedef 		typename Map::const_pointer			 const_pointer;
		typedef			typename Map::request_type			 request_type;
		typedef			typename Map::key_type			     key_type;

		// ******************************************************************
		// members
		// ******************************************************************

	public:

		smart_ptr<Map>           _sp_map;
		smart_ptr<iterator>      _sp_iterator;
		smart_ptr<request_type>  _sp_request;

    map_iterator(): _sp_map( NULL  ),
			_sp_iterator( NULL ), _sp_request( NULL ) {}

		map_iterator( Map* p_map): _sp_map( p_map ),
			_sp_iterator( NULL ), _sp_request( NULL ) {}

		map_iterator( Map* p_map, iterator* iter ) : _sp_map( p_map ),
			_sp_iterator( iter ), _sp_request( NULL ) {}

		map_iterator( Map* p_map, iterator* iter, request_type* p_request ) :
			_sp_map( p_map ), _sp_iterator( iter ), _sp_request( p_request ) {}

		map_iterator( const _Self& iter ) : _sp_request( NULL )
		{
			if( iter._sp_request.ptr ) iter._sp_request->wait();
			_sp_map = iter._sp_map;
			if( iter._sp_iterator.ptr )
				_sp_iterator = new iterator( *iter._sp_iterator );
			else
				_sp_iterator = NULL;
		}

		virtual ~map_iterator()
		{
			STXXL_ASSERT( !_sp_request.ptr || _sp_request->poll() );
			if( _sp_request.ptr ) _sp_request->wait();
		}

		_Self & operator= ( const _Self & iter )
		{
			STXXL_ASSERT( !_sp_request.ptr || _sp_request->poll() );

   if( iter._sp_request.ptr ) iter._sp_request->wait();

			_sp_map = iter._sp_map;
			_sp_request.release();

			_sp_iterator = new iterator( *iter._sp_iterator );
			return *this;
		}

		void wait()
		{
			if ( _sp_request.ptr ) _sp_request->wait();
		}

		bool poll()
		{
			if ( _sp_request.ptr ) return _sp_request->poll();
			else return true;
		}

		reference   operator* ()
		{
			if ( _sp_request.ptr ) _sp_request->wait();
			return _sp_iterator->operator* ();
		}
		
		pointer     operator->()
		{
			if ( _sp_request.ptr ) _sp_request->wait();
			return _sp_iterator->operator->();
		}

		// const_reference operator* () const { return _sp_iterator->operator* (); }
		// const_pointer   operator->() const { return _sp_iterator->operator->(); }

		_Self& operator ++()
		{
			if ( _sp_request.ptr ) _sp_request->wait();
			_sp_request = anext();
			return *this;
		}

		_Self& operator --()
		{
			if ( _sp_request.ptr ) _sp_request->wait();
			_sp_request = aprev();
			return *this;
		}

		_Self operator ++(int)
		{
			_Self iter( *this );
			++(*this);
			return iter;
		}

		_Self operator --(int)
		{
			_Self iter = *this;
			--(*this);
			return iter;
		}

		bool operator ==(const _Self & iter) const
		{
			if ( _sp_request.ptr ) _sp_request->wait();
			if ( iter._sp_request.ptr ) iter._sp_request->wait();
			return *_sp_iterator == *iter._sp_iterator && _sp_map.get() == iter._sp_map.get();
		}

		void dump( std::ostream & _str )
		{
			_sp_iterator->dump( _str );
		}

		bool valid() { return _sp_iterator->valid() ; }

	private:

		smart_ptr<request_type> anext( map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			//create new request
			smart_ptr<request_type> sp_request = new request_type( btree_request_base::NEXT, _sp_map.get(), on_compl );

			// set param for request
			btrequest_next<btree_type>* p_param = (btrequest_next<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = &(this->_sp_iterator.ptr);

			// create new lock
			key_type key = (**this->_sp_iterator.ptr).first;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) _sp_map.ptr, map_lock<_Self>::CLOSE_OPEN, key, key, map_lock<_Self>::X );

			// send request to map
			_sp_map->_map_queue.add_request( sp_request );
			return sp_request;
		}

		smart_ptr<request_type> aprev( map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
			//create new request
			smart_ptr<request_type> sp_request = new request_type(	btree_request_base::PREV, _sp_map.get(), on_compl );

			btrequest_prev<btree_type>* p_param = (btrequest_prev<btree_type>*) sp_request->_sp_int_request.get();
			p_param->_pp_iterator = &(this->_sp_iterator.ptr);

			// create new lock
			key_type key = (**this->_sp_iterator.ptr).first;
			p_param->_p_lock = new map_lock<_Self>( (unsigned long) _sp_map.ptr, map_lock<_Self>::OPEN_CLOSE, key, key, map_lock<_Self>::X );

			// send request to map
			_sp_map->_map_queue.add_request( sp_request );
			return sp_request;
		}

	};

	template<typename Map>
	bool operator ==( const map_iterator<Map>& x, const map_iterator<Map>& y)
	{
		return x.operator==( y );
	}

	template<typename Map>
	bool operator !=( const map_iterator<Map>& x, const map_iterator<Map>& y)
	{
		return !x.operator==( y );
	}

	template<typename Map>
	std::ostream& operator <<( std::ostream & _str , const map_iterator<Map>& x )
	{
		_str << *x._sp_iterator.ptr ;
		return _str;
	}


	//! The const_iterator of \c stxxl::map container.
	template<typename Map>
	class const_map_iterator : public map_pram_interface // : public ref_counter< const_map_iterator<Map> >
	{
		typedef const_map_iterator<Map> _Self;
		typedef map_iterator<Map>       _NoConstIterator;
		friend class ref_counter< const_map_iterator<Map> >;
	public:

		typedef			typename Map::_BTreeImpl            btree_type;
		typedef			typename btree_type::iterator       iterator;
		typedef			typename btree_type::const_iterator const_iterator;
		typedef 		typename Map::const_reference       const_reference;
		typedef 		typename Map::const_pointer         const_pointer;
		typedef			typename Map::request_type          request_type;
		typedef			typename Map::key_type              key_type;

		const Map*                   _p_map;
		smart_ptr<const_iterator>    _sp_const_iterator;
		smart_ptr<request_type>      _sp_request;

public:

		Map* map() { return _sp_map.get(); }

		const_map_iterator() : _p_map( NULL  ),
			_sp_const_iterator( NULL ), _sp_request( NULL) {}

		const_map_iterator( const Map* p_map) : _p_map( p_map ),
			_sp_const_iterator( NULL ), _sp_request( NULL) {}

		const_map_iterator( const Map* p_map, const_iterator* citer, request* p_request ) :
			_p_map( p_map ), _sp_const_iterator( citer ), _sp_request( p_request )  {}

		const_map_iterator( const _Self& iter )
		{

			if( _sp_request.ptr ) _sp_request->wait();
			if( iter._sp_request.ptr ) iter._sp_request->wait();

			_p_map = iter._p_map;
			_sp_request = iter._sp_request;

			if( iter._sp_const_iterator.ptr )
				_sp_const_iterator = new const_iterator( *iter._sp_const_iterator.ptr );
			else
				_sp_const_iterator = NULL;
		}

		const_map_iterator( const _NoConstIterator& iter )
		{
			if( _sp_request.ptr ) _sp_request->wait();
			if( iter._sp_request.ptr ) iter._sp_request->wait();

			_p_map = iter._sp_map.ptr;
			_sp_request = iter._sp_request;

			if( iter._sp_iterator.ptr )
				_sp_const_iterator = new const_iterator( *iter._sp_iterator.ptr );
			else
				_sp_const_iterator = NULL;
		}

		virtual ~const_map_iterator() {}

		_Self & operator= ( const _Self & iter )
		{
			if( _sp_request.ptr ) _sp_request->wait();
			if( iter._sp_request.ptr ) iter._sp_request->wait();

			_p_map = iter._p_map;
			_sp_request = iter._sp_request;

			if( iter._sp_const_iterator.ptr )
				_sp_const_iterator = new const_iterator( *iter._sp_const_iterator.ptr );
			else
				_sp_const_iterator = NULL;
			return *this;
		}

		const_reference operator*() const
		{
			if( _sp_request.ptr ) _sp_request->wait();
			return _sp_const_iterator->operator* ();
		}

		const_pointer  operator->() const
		{
			if( _sp_request.ptr ) _sp_request->wait();
			return _sp_const_iterator->operator->();
		}

		void wait()
		{
			if ( _sp_request.ptr ) _sp_request->wait();
		}

		bool poll()
		{
			if ( _sp_request.ptr ) return _sp_request->poll();
			else return true;
		}


		smart_ptr<request_type> anext( map_completion_handler on_compl = stxxl::default_map_completion_handler() )
		{
		// create new request
		smart_ptr<request_type> sp_request = new request_type(	btree_request_base::CONST_NEXT, _p_map, on_compl );

		// set param for request
		btrequest_const_next<btree_type>* p_param = (btrequest_const_next<btree_type>*) sp_request->_sp_int_request.get();
		p_param->_pp_const_iterator = &(this->_sp_const_iterator.ptr);

		// create new lock
		key_type key = (**this->_sp_const_iterator.ptr).first;
		p_param->_p_lock = new map_lock<_Self>( (unsigned long) _p_map, map_lock<_Self>::CLOSE_OPEN, key, key, map_lock<_Self>::S );

		// send request to map
		_p_map->_map_queue.add_request( sp_request );
		return sp_request;
	}

	smart_ptr<request_type> aprev( map_completion_handler on_compl = stxxl::default_map_completion_handler() )
	{
		// create new request
		smart_ptr<request_type> sp_request = new request_type(	btree_request_base::CONST_PREV, _p_map, on_compl );

		// set param for request
		btrequest_const_prev<btree_type>* p_param = (btrequest_const_prev<btree_type>*) sp_request->_sp_int_request.get();
		p_param->_pp_const_iterator = &(this->_sp_const_iterator.ptr);

		// create new lock
		key_type key = (**this->_sp_const_iterator.ptr).first;
		p_param->_p_lock = new map_lock<_Self>( (unsigned long) _p_map, map_lock<_Self>::OPEN_CLOSE, key, key, map_lock<_Self>::S );

		// send request to map
		_p_map->_map_queue.add_request( sp_request );
		return sp_request;
	}

	_Self& operator ++() // Achtung synchron
	{
		if ( _sp_request.ptr ) _sp_request->wait();
		_sp_request = anext();
		return *this;
	}

	_Self& operator --()
	{
		if ( _sp_request.ptr ) _sp_request->wait();
		_sp_request = aprev();
		return *this;
	}

	_Self operator ++(int)
	{
		_Self iter( *this );
		++(*this);
		return iter;
	}

	_Self operator --(int)
	{
		_Self iter = *this;
		--(*this);
		return iter;
	}

	bool operator ==(const _Self & iter) const
	{
		if( _p_map != iter._p_map ) return false;
		
		if( _sp_request.ptr ) _sp_request->wait();
		if( iter._sp_request.ptr ) iter._sp_request->wait();

		return *_sp_const_iterator == *iter._sp_const_iterator;
	}
};

template<typename Map>
bool operator ==( const const_map_iterator<Map>& x, const const_map_iterator<Map>& y)
{
	return x.operator==( y );
}

template<typename Map>
bool operator !=( const const_map_iterator<Map>& x, const const_map_iterator<Map>& y)
{
	return !x.operator==( y );
}


////////////////////////////////////////////////////////////////////////////////////
// ****************************************************************************** //
// *
// *  CLASSE OPTIMISTIC MAP ITERATOR:
// *
// *
// *
// *
// *
// ****************************************************************************** //
////////////////////////////////////////////////////////////////////////////////////

	template<typename Map>
	class optimistic_map_iterator : public ref_counter< optimistic_map_iterator<Map> >
	{

		typedef optimistic_map_iterator<Map>     _Self;
		typedef map_iterator<Map>                _NoOptimisticIterator;

	public:

		typedef			typename Map::_BTreeImpl           btree_type;
		typedef			typename btree_type::iterator      iterator;
		typedef			typename btree_type::request_type  request_type;
		typedef 		typename Map::reference            reference;
		typedef 		typename Map::const_reference      const_reference;
		typedef 		typename Map::pointer              pointer;
		typedef 		typename Map::const_pointer        const_pointer;
		typedef			typename Map::key_type             key_type;

		smart_ptr<Map>             _sp_map;
		smart_ptr<iterator>        _sp_iterator;
		map_lock<Map>*             _p_lock;

	public:

		Map* map() { return _sp_map.get(); }

		optimistic_map_iterator( map_lock<Map>* p_lock = NULL ) :
			_sp_map( NULL  ), _sp_iterator( NULL ), _p_lock( p_lock ) {}

		optimistic_map_iterator( Map* p_map, map_lock<Map>* p_lock = NULL ) :
			_sp_map( p_map ), _sp_iterator( NULL ), _p_lock( p_lock ) {}

		optimistic_map_iterator( Map* p_map, iterator* iter, map_lock<Map>* p_lock = NULL ) :
			_sp_map( p_map ), _sp_iterator( iter ) {}

		optimistic_map_iterator( const _Self& iter ):
			_sp_map( iter._sp_map ), _sp_iterator( iter._sp_iterator ), _p_lock( iter._p_lock ){}

		optimistic_map_iterator( const _NoOptimisticIterator& iter, map_lock<Map>* p_lock = NULL ) :
			_sp_map( iter._sp_map ), _sp_iterator( iter._sp_iterator ), _p_lock( p_lock ) {}

		~optimistic_map_iterator() {}

		reference   operator* () { return _sp_iterator->operator* (); }
		pointer     operator->() { return _sp_iterator->operator->(); }

		const_reference operator* () const { return _sp_iterator->operator* (); }
		const_pointer   operator->() const { return _sp_iterator->operator->(); }

		_Self &     operator= ( const _Self & iter )
		{
			_sp_map       = iter._sp_map;
			_sp_iterator  = iter._sp_iterator;
			return *this;
		}

		smart_ptr<request_type> anext( btree_completion_handler on_compl = stxxl::default_bree_completion_handler() )
		{
			btrequest_next<btree_type>* p_param = new btrequest_next<btree_type>( _sp_map->tree(), on_compl );
			p_param->_pp_iterator = &(this->_sp_iterator.ptr);
			p_param->_p_lock = _p_lock;
			smart_ptr<request_type> sp_request = p_param;
			_sp_map->tree()->_btree_queue.add_request( sp_request.get() );
			return sp_request;
		}

		smart_ptr<request_type> aprev( btree_completion_handler on_compl = stxxl::default_bree_completion_handler() )
		{
			btrequest_prev<btree_type>* p_param = new btrequest_prev<btree_type>( _sp_map->tree(), on_compl );
			p_param->_pp_iterator = &( this->_sp_iterator.ptr );
			p_param->_p_lock = _p_lock;
			smart_ptr<request_type> sp_request = p_param;
			_sp_map->tree()->_btree_queue.add_request( sp_request.get() );
			return sp_request;
		}

		_Self& operator ++() // Achtung synchron
		{
			smart_ptr<request_type> sp_req = anext();
			sp_req->wait();
			return *this;
		}

		_Self& operator --()
		{
			smart_ptr<request_type> sp_req = aprev();
			sp_req->wait();
			return *this;
		}

		_Self operator ++(int)
		{
			_Self iter( *this );
			++(*this);
			return iter;
		}

		_Self operator --(int)
		{
			_Self iter = *this;
			--(*this);
			return iter;
		}

		bool operator ==(const _Self & iter) const
		{
			return *_sp_iterator == *iter._sp_iterator && _sp_map.get() == iter._sp_map.get();
		}

	};

	template<typename Map>
	bool operator ==( const optimistic_map_iterator<Map>& x, const optimistic_map_iterator<Map>& y)
	{
		return x.operator==( y );
	}

	template<typename Map>
	bool operator !=( const optimistic_map_iterator<Map>& x, const optimistic_map_iterator<Map>& y)
	{
		return !x.operator==( y );
	}

////////////////////////////////////////////////////////////////////////////////////
// ****************************************************************************** //
// *
// *  CLASSE OPTIMISTIC CONST MAP ITERATOR:
// *
// *
// *
// *
// *
// ****************************************************************************** //
////////////////////////////////////////////////////////////////////////////////////

	template<typename Map>
	class optimistic_const_map_iterator : public ref_counter< optimistic_const_map_iterator<Map> >
	{

		typedef optimistic_const_map_iterator<Map>  _Self;
		typedef const_map_iterator<Map>             _NoOptimisticIterator;

	public:

		typedef			typename Map::_BTreeImpl           btree_type;
		typedef			typename btree_type::iterator      iterator;
		typedef			typename btree_type::iterator      const_iterator;
		typedef			typename btree_type::request_type  request_type;
		typedef 		typename Map::const_reference      const_reference;
		typedef 		typename Map::const_pointer        const_pointer;
		typedef			typename Map::key_type             key_type;

		smart_ptr<Map>             _sp_map;
		smart_ptr<const_iterator>  _sp_const_iterator;
		map_lock<Map>*             _p_lock;

	public:

		Map* map() { return _sp_map.get(); }

		optimistic_const_map_iterator( map_lock<Map>* p_lock = NULL ) :
			_sp_map( NULL  ), _sp_const_iterator( NULL ), _p_lock( p_lock ) {}

		optimistic_const_map_iterator( Map* p_map, map_lock<Map>* p_lock = NULL ) :
			_sp_map( p_map ), _sp_const_iterator( NULL ), _p_lock( p_lock ) {}

		optimistic_const_map_iterator( Map* p_map, iterator* iter, map_lock<Map>* p_lock = NULL ) :
			_sp_map( p_map ), _sp_const_iterator( iter ) {}

		optimistic_const_map_iterator( const _Self& iter ):
			_sp_map( iter._sp_map ), _sp_const_iterator( iter._sp_iterator ), _p_lock( iter._p_lock ){}

		optimistic_const_map_iterator( const _NoOptimisticIterator& iter, map_lock<Map>* p_lock = NULL ) :
			_sp_map( iter._sp_map ), _sp_const_iterator( iter._sp_iterator ), _p_lock( p_lock )
		{
			STXXL_MSG( "*************************************" );
		}
		~optimistic_const_map_iterator() {}

		const_reference operator* () const { return _sp_iterator->operator* (); }
		const_pointer   operator->() const { return _sp_iterator->operator->(); }

		_Self &     operator= ( const _Self & iter )
		{
			_sp_map       = iter._sp_map;
			_sp_iterator  = iter._sp_iterator;
			return *this;
		}

		smart_ptr<request_type> anext( btree_completion_handler on_compl = stxxl::default_bree_completion_handler() )
		{
			btrequest_const_next<btree_type>* p_param = new btrequest_const_next<btree_type>( _sp_map->tree(), on_compl );
			p_param->_pp_iterator = &(this->_sp_iterator.ptr);
			p_param->_p_lock = _p_lock;
			smart_ptr<request_type> sp_request = p_param;
			_sp_map->tree()->_btree_queue.add_request( sp_request.get() );
			return sp_request;
		}

		smart_ptr<request_type> aprev( btree_completion_handler on_compl = stxxl::default_bree_completion_handler() )
		{
			btrequest_const_prev<btree_type>* p_param = new btrequest_const_prev<btree_type>( _sp_map->tree(), on_compl );
			p_param->_pp_iterator = &( this->_sp_iterator.ptr );
			p_param->_p_lock = _p_lock;
			smart_ptr<request_type> sp_request = p_param;
			_sp_map->tree()->_btree_queue.add_request( sp_request.get() );
			return sp_request;
		}

		_Self& operator ++() // Achtung synchron
		{
			smart_ptr<request_type> sp_req = anext();
			sp_req->wait();
			return *this;
		}

		_Self& operator --()
		{
			smart_ptr<request_type> sp_req = aprev();
			sp_req->wait();
			return *this;
		}

		_Self operator ++(int)
		{
			_Self iter( *this );
			++(*this);
			return iter;
		}

		_Self operator --(int)
		{
			_Self iter = *this;
			--(*this);
			return iter;
		}

		bool operator ==(const _Self & iter) const
		{
			return *_sp_iterator == *iter._sp_iterator && _sp_map.get() == iter._sp_map.get();
		}

	};

	template<typename Map>
	bool operator ==( const optimistic_const_map_iterator<Map>& x, const optimistic_const_map_iterator<Map>& y)
	{
		return x.operator==( y );
	}

	template<typename Map>
	bool operator !=( const optimistic_const_map_iterator<Map>& x, const optimistic_const_map_iterator<Map>& y)
	{
		return !x.operator==( y );
	}

};
