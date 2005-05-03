/***************************************************************************
                          my_type.h  -  description
                             -------------------
    begin                : Son Mär 7 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file my_type.h
//! \brief This file contains the datatyps can be used for testing of \c stxxl::map container.

#ifndef CONTAINER__MY_TYPE_H
#define CONTAINER__MY_TYPE_H

#define MIN_unsigned	0x00000000
#define MAX_unsigned	0xFFFFFFFF


typedef stxxl::uint64 key_type;
typedef stxxl::uint64 data_type;

namespace stxxl
{

//! \brief namespace contain structures and rules for testing the \c stxxl::map container.
namespace map_test
{
	/********** MY_TYPE ************************************************************/

	//! \brief this type will be user in all tests and examples.
	/*!
	The test data type for whole tests derived from STL's \c std::pair.
	We define an order on this data type in stuctures cmp and cmp2.
	*/
	struct my_type : public std::pair<key_type,data_type>
	{
		// ***********************************************************************
		// diferent constructors
		// ***********************************************************************

		my_type() {};
		my_type( key_type __key, data_type __data ) :
			std::pair<key_type,data_type>(__key,__data) {};

		my_type( const my_type& x ) :
			std::pair<key_type,data_type>( x.first, x.second ) {};

		my_type( const std::pair<const key_type,data_type>& x ) :
			std::pair<key_type,data_type>( x.first, x.second ) {};

		// ***********************************************************************
		// member access
		// ***********************************************************************

		key_type key() const { return first; };
		key_type data() const { return second; };

		// static my_type min_value(){ return my_type(0,0); };
		// static my_type max_value(){ return my_type(0xffffffff,0); };

		bool equal( const my_type & __X )
		{
			return first == __X.first && second == __X.second;
		}
		bool equal( const std::pair<const key_type,data_type> & __X )
		{
			return first == __X.first && second == __X.second;
		}
	};

	//! \brief This class implement a cheep iterator.
	/*!
	This iterator we can use for testing the bulk operations of stxxl::map.
	All Operations needs O(1) time.
	*/
	class my_iterator
  {

    stxxl::int64 _pos;

  public:

    my_iterator( stxxl::int64 pos = -1 ): _pos(pos) {}
    my_iterator( const my_iterator& i ): _pos(i._pos) {}

    my_iterator& operator=( const my_iterator& i )
    {
      _pos = i._pos;
      return *this;
    }
    
    my_iterator operator++(int)
    {
      my_iterator i(_pos++); return i;
    }

    my_iterator operator++()
    {
      ++_pos; return *this;
    }

    my_iterator operator+=( stxxl::int64 o )
    {
      _pos += o;
      return *this;
    }

    my_iterator operator--(int)
    {
      my_iterator i(_pos--); return i;
    }

    my_iterator operator--()
    {
      --_pos; return *this;
    }

    my_iterator operator-=( stxxl::int64 o )
    {
      _pos -= o;
      return *this;
    }

    my_type operator*()
    {
      return my_type(_pos,_pos);
    }
    
    friend stxxl::int64 operator-( const my_iterator& i1, const my_iterator& i2 );
    friend my_iterator operator+( const my_iterator& i1, stxxl::int64 o );
    friend my_iterator operator-( const my_iterator& i1, stxxl::int64 o );
    friend bool operator==( const my_iterator& i1, const my_iterator& i2 );
    friend bool operator<( const my_iterator& i1, const my_iterator& i2 );
    friend bool operator<=( const my_iterator& i1, const my_iterator& i2 );
    friend bool operator>( const my_iterator& i1, const my_iterator& i2 );
    friend bool operator>=( const my_iterator& i1, const my_iterator& i2 );
    friend bool operator!=( const my_iterator& i1, const my_iterator& i2 );
  }; // class my_iterator

  inline
  bool operator==( const my_iterator& i1, const my_iterator& i2 )
  {
    return i1._pos == i2._pos ;
  }

  inline
  bool operator<( const my_iterator& i1, const my_iterator& i2 )
  {
    return i1._pos < i2._pos ;
  }

  inline
  bool operator<=( const my_iterator& i1, const my_iterator& i2 )
  {
    return i1._pos <= i2._pos ;
  }

  inline
  bool operator>( const my_iterator& i1, const my_iterator& i2 )
  {
    return i1._pos > i2._pos;
  }

  inline
  bool operator>=( const my_iterator& i1, const my_iterator& i2 )
  {
    return i1._pos >= i2._pos ;
  }

  inline
  bool operator!=( const my_iterator& i1, const my_iterator& i2 )
  {
    return i1._pos != i2._pos ;
  }

  stxxl::int64 operator-( const my_iterator& i1, const my_iterator& i2 )
  {
    return i1._pos - i2._pos;
  }

  my_iterator operator-( const my_iterator& i1, stxxl::int64 o )
  {
    return my_iterator ( i1._pos - o );
  }

  my_iterator operator+( const my_iterator& i1, stxxl::int64 o )
  {
    return my_iterator ( i1._pos + o );
  }

  class my_insert
  {
    stxxl::int64 _end;
    stxxl::int64 _begin;

  public:
    typedef my_iterator iterator;
    my_insert( stxxl::int64 begin, stxxl::int64 end )
    {
      _end = end;
      _begin = begin;
    }

    my_insert( long begin, long end )
    {
      _end = end;_begin = begin;
    }

    iterator begin() { return iterator( _begin ); }
    iterator end() { return iterator( _end ); }
  };

	bool operator < (const my_type & a, const my_type & b)
	{
		return a.key() < b.key();
	}

	std::ostream& operator <<( std::ostream & _str , const my_type & x )
	{
		_str << "<" << x.first << " :: " << x.second << ">";
		return _str;
	}

	std::ostream& operator <<( std::ostream & _str , const std::pair<const key_type,data_type> & x )
	{
		_str << "<" << x.first << " :: " << x.second << ">";
		return _str;
	}

	//! \brief The comperator for \c stxxl::map_test::my_type.
	struct cmp: public std::less<my_type>
	{
		my_type min_value() const { return my_type(0,0); };
		my_type max_value() const { return my_type(0xffffffff,0); };
	};

	//! \brief The comperator for \c stxxl::map_test::my_type.
	struct cmp2: public std::less<key_type>
	{
		key_type min_value() const { return key_type(0); };
		key_type max_value() const { return key_type(0xffffffff); };
	};

	cmp2 g_cmp2;

	cmp2& CreateReference( cmp2* p, void *parent, int ix )
	{
		return g_cmp2;
	}

	//! \brief A class for a key can be a string.
	class str50
	{
		char _p_str[51];

	public:
		str50()
		{
			_p_str[0] = '\0';
		}

		str50( char* p )
		{
			memcpy( _p_str, p, 50 );
			_p_str[50] = '\0';
		}

		str50( const str50& s )
		{
			memcpy( _p_str, s._p_str, 51 );
		}

		str50& operator= ( const str50& s )
		{
			memcpy( _p_str, s._p_str, 51 );
			return *this;
		}

		std::ostream& operator<< ( std::ostream& ostr ) const
		{
			ostr << _p_str;
			return ostr;
		}
	};

	std::ostream& operator<< ( std::ostream& ostr, const str50& str )
	{
		return str.operator<<( ostr );
	};

}// namespace map_test
}// namespace stxxl

#endif
