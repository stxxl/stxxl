/***************************************************************************
                          btree_iterator.h  -  description
                             -------------------
    begin                : Sam Feb 12 2005
    copyright            : (C) 2005 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file btree_iterator.h
//! \brief Implemets the btree_iterator and const_btree_iterator for the \c stxxl::map_internal::btree - B-Tree implemantation for \c stxxl::map container.


__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	template<typename Key_,typename Data_,typename Compare_,unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_> class btree;
	template<typename Key_,typename Data_,typename Compare_,unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_> class btree_iterator;
	template<typename Key_,typename Data_,typename Compare_,unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_> class const_btree_iterator;

	
 	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	class btree_iterator : public ref_counter<btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> >
	{
		typedef 		btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> _Self;

	public:
		typedef			btree<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> btree_type;
		typedef			typename btree_type::_Node							 node_type;
		typedef			typename btree_type::node_iterator			 node_iterator;
		typedef			typename btree_type::_Leaf							 leaf_type;
		typedef			typename btree_type::leaf_iterator			 leaf_iterator;
		typedef 		typename btree_type::key_type						 key_type;
		typedef 		typename btree_type::mapped_type				 mapped_type;
		typedef 		typename btree_type::value_type					 value_type;
		typedef 		typename btree_type::store_type					 store_type;
		typedef 		typename btree_type::size_type					 size_type;
		typedef 		typename btree_type::difference_type		 difference_type;
		typedef 		typename btree_type::reference  	  		 reference;
		typedef 		typename btree_type::const_reference  	 const_reference;
		typedef 		typename btree_type::pointer						 pointer;
		typedef 		typename btree_type::const_pointer			 const_pointer;

		// todo: make it private

		btree_type* _p_btree ;
		std::vector<node_iterator> _node_iterators ;
		leaf_iterator _leaf_iterator ;

	protected:
		btree_iterator(
			btree_type* p_btree,
			const std::vector<node_iterator>& node_iter,
			const leaf_iterator leaf_iter ) :
		_p_btree( p_btree ), _node_iterators(node_iter), _leaf_iterator(leaf_iter) {}

		btree_iterator( btree_type* p_btree ) : _p_btree( p_btree ) {}

	public:

		~btree_iterator(){}
		btree_iterator() : _p_btree( NULL ){}

		btree_iterator( const _Self& iter ) :
			_p_btree( iter._p_btree ),
			_node_iterators( iter._node_iterators ),
			_leaf_iterator( iter._leaf_iterator ) {}

	private:
		unsigned height() { return _node_iterators.size() + 1; }

	public:
		reference				operator*()           { return *_leaf_iterator; }
		pointer					operator->()          { return _leaf_iterator.operator->(); }
		const_reference operator*() const     { return *_leaf_iterator; }
		const_pointer 	operator->() const    { return _leaf_iterator.operator->(); }

		/*
		difference_type operator -( const _Self & iter ) const
		{
			STXXL_ASSERT( _p_btree == iter._p_btree );

			if( _leaf_iterator.bid() == iter._leaf_iterator.bid() )
				return _leaf_iterator.offset() - iter._leaf_iterator.offset();

			size_type size = 0;
			leaf_iterator l_iter = _leaf_iterator;
			while(
				!( l_iter.bid() == iter._leaf_iterator.bid())
				&& l_iter != leaf_type::nil.end() )
			{
				l_iter.next();
				size++;
			}
			return size + l_iter.offset() - iter._leaf_iterator.offset() ;
		}

		_Self& operator +( difference_type size )
		{
			STXXL_ASSERT( size >= 0 );
			while( size-- ) operator++();
			return *this;
		}*/

		_Self & operator =(	const _Self & iter )
		{
			_p_btree = iter._p_btree;
			_node_iterators = iter._node_iterators;
			_leaf_iterator = iter._leaf_iterator;
			return *this;
		}

		//_Self 			operator -( size_type offset )		{ return _Self( _p_node, _n_offset - offset); }
		//_Self 			operator +( size_type offset )		{ return _Self( _p_node, _n_offset + offset); }

		bool _next( leaf_iterator& iter )
		{
			STXXL_ASSERT( iter._sp_node.get() );

			bool ret = false;
			if ( ++_n_offset >= iter._sp_node->size() )
			{
				smart_ptr<leaf_type> sp_node_alt = iter._sp_node;
				iter._sp_node = iter._sp_node->next();
				if ( iter._sp_node.get() ) ret = true;
				else iter._sp_node = &node_type::nil;
				iter._bid = iter._sp_node->bid() ;
				iter._n_offset = 0;
			}

			STXXL_ASSERT( iter.valid() );
			return ret;
		}

		bool _next( node_iterator& iter )
		{
			STXXL_ASSERT( iter._sp_node.get() );

			bool ret = false;
			if ( ++_n_offset >= iter._sp_node->size() )
			{
				smart_ptr<node_type> sp_node_alt = iter._sp_node;
				iter._sp_node = iter._sp_node->next();
				if ( iter._sp_node.get() ) ret = true;
				else iter._sp_node = &node_type::nil;
				iter._bid = iter._sp_node->bid() ;
				iter._n_offset = 0;
			}

			STXXL_ASSERT( iter.valid() );
			return ret;
		}

		bool prev( leaf_iterator& iter );
		bool prev( node_iterator& iter );

	_Self& operator ++()
	{
		if( _leaf_iterator.next() )
			return next( _p_btree->height() - 2 ) ;
		return *this;
	}

	_Self& next( int index )
	{
		if ( _node_iterators[index].next() ) return next( --index );
		return *this;
	}

	_Self& operator --()
	{
		if( _leaf_iterator.prev() )
			return prev( _p_btree->height() - 2 );
		return *this;
	}

	_Self& prev( int index )
	{
//		if ( _node_iterators[index].prev() )
			return prev( --index );
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

		void dump( std::ostream & _str )
	{
		if ( !_p_btree )
		{
			_str << "tree::NULL" << "\n";
		}

		unsigned ix = _p_btree->height() - 1;
		_leaf_iterator.node()->dump( _str );
		while( ix-- )
		{
			if ( _node_iterators.size() <= ix )
				_str << ix << ": ------ " << "\n";
			else
				_str << ix << ":";
				_node_iterators[ix].node()->dump( _str );
		}
	}


	std::ostream& operator <<( std::ostream & _str )
	{
		if ( !_p_btree )
		{
			_str << "tree::NULL" << "\n";
			return _str;
		}

		int ix = _p_btree->height() - 1;
		_str << _leaf_iterator << "\n" ;
		while( ix-- )
		{
			if ( _node_iterators.size() <= (unsigned) ix )
				_str << ix << ": ------ " << "\n";
			else
				_str << ix << ":" << _node_iterators[ix] << "\n";
		}
		return _str;
	}

	bool valid()
	{
		if ( *this == _p_btree->end() ) return true;

		int ix = _p_btree->height() - 1;
		BID<SIZE_> bid1 = _leaf_iterator.bid() ;
		while( ix-- > 0 )
		{
			BID<SIZE_> bid2 = (*_node_iterators[ix]).second ;
			if( !(bid1 == bid2) )
			{
				STXXL_MSG( bid1 << "!=" << bid2 ) ;
				// STXXL_ASSERT( false ) ;
				return false ;
			}
			bid1 = _node_iterators[ix].bid();
		}
		return true;
	}

	bool 			operator ==(const _Self & iter) const
	{
		return _leaf_iterator == iter._leaf_iterator && _p_btree == iter._p_btree;
	}

	bool 			operator !=(const _Self & iter) const
	{
		return !operator==( iter );
	}

	bool 			operator < (const _Self & iter) const
	{
		return ( _p_btree == iter._p_btree && _leaf_iterator < iter._leaf_iterator);
	}

	friend class btree<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>;
}; // class btnode_iterator


 	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
  bool btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::prev( leaf_iterator& iter )
		{
			bool ret = false;
			STXXL_ASSERT( iter._sp_node.get() );
			if ( iter._n_offset == 0 )
			{
				smart_ptr<leaf_type> sp_node_alt = iter._sp_node;
				iter._sp_node = iter._sp_node->prev();
				if ( iter._sp_node.get() )
				{
					iter._n_offset = iter._sp_node->size() - 1;
					ret = true;
				}
				else
				{
					iter._sp_node = node_type::nil.add_ref();
					iter._n_offset = 0;
				}
				iter._bid = iter._sp_node->bid() ;
			}
			else
				--iter._n_offset ;
			STXXL_ASSERT( iter.valid() );
			return ret;
		}


 	template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
	bool btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>::prev( node_iterator& iter )
	{
		bool ret = false;
		STXXL_ASSERT( iter._sp_node.get() );
		if ( iter._n_offset == 0 )
		{
			smart_ptr<node_type> sp_node_alt = iter._sp_node;
			iter._sp_node = iter._sp_node->prev();
			if ( iter._sp_node.get() )
			{
				iter._n_offset = iter._sp_node->size() - 1;
				ret = true;
			}
			else
			{
				iter._sp_node = node_type::nil.add_ref();
				iter._n_offset = 0;
			}
			iter._bid = iter._sp_node->bid() ;
		}
		else
			--iter._n_offset ;
		STXXL_ASSERT( iter.valid() );
		return ret;
	}


template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
class const_btree_iterator : public ref_counter<const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> >
{
// ***********************************
// TYPEDEF

private:
	typedef 		const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_> _Self;
	typedef			btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>			_NoConstIterator;
	typedef			btree<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>							btree_type;

// ***********************************


// ***********************************
// TYPENAME

	typedef 		typename btree_type::const_leaf_iterator	leaf_iterator;

public:

	typedef			typename btree_type::_Leaf								leaf_type;
	typedef 		typename btree_type::key_type							key_type;
	typedef 		typename btree_type::mapped_type					mapped_type;
	typedef 		typename btree_type::value_type						value_type;
	typedef 		typename btree_type::store_type					  store_type;
	typedef 		typename btree_type::size_type						size_type;
	typedef 		typename btree_type::difference_type			difference_type;
	typedef 		typename btree_type::const_reference  		const_reference;
	typedef 		typename btree_type::const_pointer				const_pointer;

// ***********************************


protected:
	const btree_type* 	_p_btree ;
	leaf_iterator _leaf_iterator ;

	const_btree_iterator( const btree_type* p_btree, leaf_iterator leaf_iterator ) :
	_p_btree( p_btree ),
	_leaf_iterator( leaf_iterator )
	{}

	const_btree_iterator( const btree_type* p_btree ) :
	_p_btree( p_btree )
	{}

public:

	const_btree_iterator() :
	_p_btree( NULL )
	{}

	const_btree_iterator( const _NoConstIterator& iter ) :
	_p_btree( iter._p_btree ),
	_leaf_iterator( iter._leaf_iterator )
	{}

	const_btree_iterator( const _Self& iter ) :
	_p_btree( iter._p_btree ),
	_leaf_iterator( iter._leaf_iterator )
	{}

	const_pointer operator->()
	{
		return _leaf_iterator.operator->();
	}

	const_reference operator*() const
	{
		return *_leaf_iterator;
	}

	difference_type operator -( const _Self & iter ) const
	{
		STXXL_ASSERT( _p_btree == iter._p_btree );
		if( _leaf_iterator.bid() == iter._leaf_iterator.bid() )
			return _leaf_iterator.offset() - iter._leaf_iterator.offset();
		size_type size = 0;
		leaf_iterator l_iter = _leaf_iterator;
		while( !( l_iter.bid() == iter._leaf_iterator.bid() ) && l_iter != leaf_type::nil.end() )
		{
			l_iter.next();
			size++;
		}
		return size + l_iter.offset() - iter._leaf_iterator.offset();
	}

	_Self& operator +( difference_type size )
	{
		STXXL_ASSERT( size >= 0 );
		while( size-- )
		{
			_leaf_iterator.next();
		}
		return *this;
	}

	_Self& operator =( const _Self & iter )
	{
		_p_btree = iter._p_btree;
		_leaf_iterator = iter._leaf_iterator;
		return *this;
	}

	_Self& operator ++()
	{
		_leaf_iterator.next();
		return *this;
	}

	_Self& operator --()
	{
		_leaf_iterator.prev();
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

	std::ostream& operator <<( std::ostream & _str )
	{
		_str << _leaf_iterator;
		return _str;
	}

	bool operator ==(const _Self & iter) const
	{
		return (_leaf_iterator == iter._leaf_iterator) && (_p_btree == iter._p_btree);
	}

	bool operator !=(const _Self & iter) const
	{
		return !operator==( iter );
	}

	bool operator < (const _Self & iter) const
	{
		return ( _p_btree == iter._p_btree && _leaf_iterator < iter._leaf_iterator);
	}

	friend class btree<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>;
}; // class btnode_iterator


template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
std::ostream& operator << ( std::ostream& _str, btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& _tree_iterator )
{
	return _tree_iterator.operator<<( _str );
}

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator==( const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return x.operator==( y ); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator== ( const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return x.operator==( y ); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator!=( const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return !( x.operator==( y) ); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator!=( const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return !( x.operator==( y) ); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator<( const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return !( x.operator<( y) ); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator<( const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return x.operator<(y); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator>( const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return y.operator<(x); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator>( const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return y.operator<( x);}

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator>=( const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return !( x.operator<( y) ); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator>=( const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return !( x.operator<( y) ); }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator<=( const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return ! y.operator<(x) ; }

template<typename Key_,typename Data_, typename Compare_, unsigned SIZE_,bool unique_,unsigned CACHE_SIZE_>
inline
bool operator<=( const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& x, const const_btree_iterator<Key_,Data_,Compare_,SIZE_,unique_,CACHE_SIZE_>& y )
{ return !( y.operator<(x ) ); }

//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE


