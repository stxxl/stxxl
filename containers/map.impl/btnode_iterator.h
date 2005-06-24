/***************************************************************************
                          btnode_iterator.h  -  description
                             -------------------
    begin                : Die Okt 12 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file btnode_iterator.h
//! \brief Implemets the iterators for cointener \c stxxl::map_internal::btnode nodes for the \c stxxl::map container.

#ifndef __STXXL_BTNODE_ITERATOR_H
#define __STXXL_BTNODE_ITERATOR_H

#include "map.h"
#define BTREE_NODE_SIZE 						4096
#define _SIZE_TYPE 									stxxl::uint64

__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	template< typename Key_,typename Data_,typename Compare_,unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_> class btnode;
	template< typename Key_,typename Data_,typename Compare_,unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_> class btnode_iterator;
	template< typename Key_,typename Data_,typename Compare_,unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_> class const_btnode_iterator;

	//! \brief An iterator for btnode container.
	/*!
	*/
	template< typename Key_,typename Data_,typename Compare_,unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	class btnode_iterator
	{
		typedef 	btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> _Self;

	public:
		typedef		btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> 	node_type;
		typedef 	typename node_type::key_type				key_type;
		typedef 	typename node_type::mapped_type			mapped_type;
		typedef 	typename node_type::value_type			value_type;
		typedef 	typename node_type::size_type				size_type;
		typedef 	typename node_type::difference_type	difference_type;
		typedef 	typename node_type::reference				reference;
		typedef 	typename node_type::pointer					pointer;

		smart_ptr<node_type> _sp_node;
		size_type _n_offset;
		stxxl::BID<SIZE_> _bid;

	protected:

	public:

		btnode_iterator() : _sp_node( NULL ),_n_offset(0),_bid()
		{
			STXXL_VERBOSE3( "btnode_iterator\t" << this << " construction, node: 0x0" );
			STXXL_ASSERT( valid() );
		}

		btnode_iterator( const _Self & iter ) : _sp_node( iter._sp_node ),_n_offset( iter._n_offset ),_bid( iter._bid )
		{
			STXXL_VERBOSE3( "btnode_iterator\t" << this << " copy, node: " << unsigned(_sp_node.get()) );
			STXXL_ASSERT( valid() );
		}

		btnode_iterator( node_type	*p_node ) : _sp_node( p_node ),_n_offset(0),_bid( p_node->_bid )
		{
			STXXL_VERBOSE3( "btnode_iterator\t" << this << " construction, node: " << unsigned(_sp_node.get()) );
			STXXL_ASSERT( valid() );
		}

		btnode_iterator( node_type	*p_node, size_type n_offset ) :_sp_node( p_node ),_n_offset(n_offset),_bid( p_node->_bid )
		{
			STXXL_VERBOSE3( "btnode_iterator\t" << this << " construction, node: " << unsigned(_sp_node.get()) );
			STXXL_ASSERT( valid() );
		}

		virtual ~btnode_iterator()
		{
			STXXL_VERBOSE3( "btnode_iterator\t" << this << " destruction, node: " << unsigned(_sp_node.get()) );
		}


	// -----------------------------------------------------------------------------
	// operators
	// -------------------------------------------------------------------------------------

		pointer operator->()
		{
			//! @ todo check for const
			STXXL_ASSERT( _sp_node.get() );
			_sp_node->_dirty = true;
			return _sp_node->_p_values + _n_offset  ;
		}

		reference operator *()
		{
			//! @ todo check for const
			STXXL_ASSERT( _sp_node.get() );
			_sp_node->_dirty = true;
			return _sp_node->_p_values[ _n_offset ] ;
		}

		difference_type operator -( const _Self & iter )
		{
			return _n_offset - iter._n_offset;
		}

		_Self& operator =(	const _Self & iter )
		{
			_sp_node = iter._sp_node ;
			_n_offset = iter._n_offset;
			_bid = iter._bid;
			STXXL_VERBOSE3( "btnode_iterator\t" << this << " operator=, node: " << unsigned(_sp_node.get()) );
			return *this;
		}

		_Self& operator +=( difference_type offset )
		{
			STXXL_ASSERT( _n_offset + offset <= _sp_node->size() );
			_n_offset += offset;
			return *this;
		}	

		bool next() { return ( ++_n_offset >= _sp_node->size() ); }
		bool prev() { return ( _n_offset-- == 0 ); }


		_Self& operator -=( difference_type offset ) { _n_offset -= offset; return *this; }
		_Self operator -( difference_type offset ) { return _Self( _p_node, _n_offset - offset); }
		_Self operator +( difference_type offset ) { return _Self( _p_node, _n_offset + offset); }
		_Self& operator ++() { _n_offset++;	return *this; }
		_Self& operator --() { _n_offset--;	return *this; }
		_Self operator ++(int) { _Self iter = *this; _n_offset++; return iter; }
		_Self operator --(int) { _Self iter = *this; _n_offset--; return iter; }

		bool operator ==(const _Self& iter) const
		{
			return
			(
				_n_offset == iter._n_offset
						&& ( _bid == iter._bid || !_bid.valid() && !iter._bid.valid()
						&& _sp_node.get() == iter._sp_node.get() )
			);
		}

		bool 			operator <(const _Self&  iter) const
		{
			return ( _sp_node.get() == iter._sp_node.get() && _n_offset < iter._n_offset);
		}

		size_type  offset() const { return _n_offset; }
		BID<SIZE_> bid()    const { return _bid; }
		node_type* node()         { return _sp_node.get(); }

		std::ostream& operator <<( std::ostream & _str ) const
		{
			if ( !_sp_node.get() )
			{
				_str << "node::NULL";
				return _str;
			}
			_str <<
				"< bid = " << _bid <<
				", offset = " << _n_offset <<
				", < key = " << _sp_node->_p_values[ _n_offset ].first <<
				" :data = " << _sp_node->_p_values[ _n_offset ].second << "> >";
			return _str;
		}

		bool valid()
		{
			STXXL_ASSERT( _n_offset >= 0 );

#ifdef STXXL_DEBUG_ON
			STXXL_ASSERT( _n_offset <= node_type::MAX_SIZE() || _sp_node->is_nil);
#else
			STXXL_ASSERT( _n_offset <= node_type::MAX_SIZE());
#endif
		

			STXXL_ASSERT( _sp_node == NULL || _n_offset <= _sp_node->size() );

			bool ret = ( _n_offset >= 0 ) && ( _n_offset <= node_type::MAX_SIZE()
#ifdef STXXL_DEBUG_ON
				|| _sp_node->is_nil )
#else
			)
#endif
				&& ( _sp_node == NULL || _n_offset <= _sp_node->size() );

			return ret;
		}
	}; // class btnode_iterator

//! \brief an iterator for btnode container.
/*!
Hier kommt mehr Text ...
*/
template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
class const_btnode_iterator
{

private:
	typedef const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>	_Self;
	typedef btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>				_NoConstIterator;

public:
	typedef 		btnode<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> node_type;
	
	typedef 		typename node_type::key_type						key_type;
	typedef 		typename node_type::mapped_type					mapped_type;
	typedef 		typename node_type::value_type					value_type;
	typedef 		typename node_type::size_type						size_type;
	typedef 		typename node_type::difference_type			difference_type;
	typedef 		typename node_type::const_reference			const_reference;
	typedef 		typename node_type::const_pointer				const_pointer;

protected:
	smart_ptr<node_type>      _sp_node;
	size_type                 _n_offset;
	stxxl::BID<SIZE_>         _bid;

public:

	const_btnode_iterator() :
		_sp_node( NULL ),
		_n_offset(0),
		_bid()
	{
		STXXL_ASSERT( valid() );
	}

	const_btnode_iterator( const _NoConstIterator& iter ) :
		_sp_node( iter._sp_node.get() ),
		_n_offset( iter._n_offset ),
		_bid( iter._bid )
	{
		STXXL_ASSERT( valid() );
	}

	const_btnode_iterator( const _Self & iter ) :
		_sp_node( iter._sp_node ),
		_n_offset( iter._n_offset ),
		_bid( iter._bid )
	{
		STXXL_ASSERT( valid() );
	}

	const_btnode_iterator( node_type *p_node ) :
		_sp_node( p_node ),
		_n_offset(0),
		_bid( p_node->bid() )
	{
		STXXL_ASSERT( valid() );
	}

	const_btnode_iterator( node_type* p_node, size_type n_offset ) :
		_sp_node( p_node ),
		_n_offset(n_offset),
		_bid( p_node->bid() )
	{
		STXXL_ASSERT( valid() );
	}

	virtual ~const_btnode_iterator()
	{
	}

	node_type* node() { return _sp_node.get(); }

	bool next()
	{
		return ( ++_n_offset >= _p_node->size() ); 
	}

	void prev()
	{
		if ( --_n_offset < 0 )
		{
			_sp_node 	= _p_node->prev();
			if ( _p_node == NULL ) _p_node = node_type::nil.AddRef();
			_bid = _p_node->bid() ;
			STXXL_ASSERT( _p_node->valid() );
			_n_offset = _p_node->size();
			if ( _n_offset ) --_n_offset;
		}
		STXXL_ASSERT( valid() );
	}

	// -----------------------------------------------------------------------------
	// operators
	// -------------------------------------------------------------------------------------

	const_reference operator *() const
	{
		STXXL_ASSERT( _sp_node.get() );
		return _sp_node->_p_values[ _n_offset ] ;
	}

	const_pointer operator->() const
	{
		STXXL_ASSERT( _sp_node.get() );
		return _sp_node->_p_values + _n_offset  ;
	}

	difference_type operator -( const _Self & iter ) const
	{
		return _n_offset - iter._n_offset;
	}

	_Self& operator =( const _Self & iter )
	{
		if ( _sp_node != iter._sp_node ) _sp_node = iter._sp_node ;
		_n_offset 	= iter._n_offset;
		_bid 		= iter._bid;
		return *this;
	}

	_Self& operator =( const _NoConstIterator & iter )
	{
		if ( _sp_node.get() != iter._sp_node.get() ) _sp_node = iter._sp_node ;
		_n_offset 	= iter._n_offset;
		_bid 		= iter._bid;
		return *this;
	}

	_Self &			operator +=( difference_type offset ) { _n_offset += offset; return *this; }
	_Self &			operator -=( difference_type offset ) { _n_offset -= offset; return *this; }
	_Self 			operator -( difference_type offset ) { return _Self( _p_node, _n_offset - offset); }
	_Self 			operator +( difference_type offset ) { return _Self( _p_node, _n_offset + offset); }
	_Self & 		operator ++() { _n_offset++;	return *this; }
	_Self & 		operator --() { _n_offset--;	return *this; }
	_Self 			operator ++(int) { _Self iter = *this; _n_offset++; return iter; }
	_Self 			operator --(int) { _Self iter = *this; _n_offset--; return iter; }

	bool operator ==(const _Self& iter) const
	{
		return ( _n_offset == iter._n_offset && _bid == iter._bid );
	}
	bool operator <(const _Self&  iter) const
	{
		return ( _p_node == iter._p_node && _n_offset < iter._n_offset);
	}

	size_type offset() const { return _n_offset; }
	BID<SIZE_> bid() const { return _bid; }

	std::ostream& operator <<( std::ostream & _str ) const
	{
		_str <<
		"NODE = " << _sp_node.get() <<
		" bid = " << _bid <<
		" position = " << _n_offset <<
		" < key = " << _sp_node->_p_values[ _n_offset ].first <<
		" :data = " << _sp_node->_p_values[ _n_offset ].second << ">";
		return _str;
	}

	bool valid()
	{
		STXXL_ASSERT( _n_offset >= 0 );
		STXXL_ASSERT( _n_offset <= node_type::MAX_SIZE() );
		STXXL_ASSERT( _sp_node == NULL || _n_offset <= _sp_node->size() );
		bool ret =
			( _n_offset >= 0 )
			&& ( _n_offset <= node_type::MAX_SIZE() )
			&& ( _sp_node == NULL || _n_offset <= _sp_node->size() );

		return ret;
	}


}; // class const_btnode_iterator

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator==( const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return x.operator==( y ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator== ( const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return x.operator==( y ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator!=( const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return !( x.operator==( y) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator!=( const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return !( x.operator==( y) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator<( const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return !( x.operator<( y) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator<( const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return x.operator<(y); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator>( const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return y.operator<(x); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator>( const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return y.operator<( x);}

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator>=( const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return !( x.operator<( y) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator>=( const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return !( x.operator<( y) ); }


	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator<=( const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return ! y.operator<(x) ; }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	inline
	bool operator<=( const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
	{ return !( y.operator<(x ) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	std::ostream& operator <<( std::ostream & _str, const btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& _node_iterator )
	{ return _node_iterator.operator<<( _str ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	std::ostream& operator <<( std::ostream & _str, const const_btnode_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& _node_iterator )
	{ return _node_iterator.operator<<( _str ); }


//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE

#endif
