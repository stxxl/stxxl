#ifndef SCAN_HEADER
#define SCAN_HEADER

/***************************************************************************
 *            scan.h
 *
 *  Tue Dec 31 20:27:24 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../common/utils.h"
#include "../mng/buf_istream.h"
#include "../mng/buf_ostream.h"

__STXXL_BEGIN_NAMESPACE

//! \brief External equivalent of std::for_each
//! \remark The implementation exploits \c \<stxxl\> buffered streams (computation and I/O overlapped)
//! \param _begin object of model of \c ext_random_access_iterator concept
//! \param _end object of model of \c ext_random_access_iterator concept
//! \param _functor function object of model of \c std::UnaryFunction concept 
//! \param nbuffers number of buffers for internal use (should be at least 2*D )
//! \return function object \c _functor after it has been applied to the each element of the given range
template <typename _ExtIterator,typename _UnaryFunction>
_UnaryFunction for_each(_ExtIterator _begin, _ExtIterator _end, _UnaryFunction _functor,int nbuffers)
{
	typedef buf_istream<typename _ExtIterator::block_type,typename _ExtIterator::bids_container_iterator> buf_istream_type;
	typedef buf_ostream<typename _ExtIterator::block_type,typename _ExtIterator::bids_container_iterator> buf_ostream_type;
	
	_begin.flush(); // flush container
	
	// create prefetching stream, 
	buf_istream_type in(_begin.bid(),_end.bid() + ((_end.block_offset())?1:0),nbuffers/2); 
	// create buffered write stream for blocks
	buf_ostream_type out(_begin.bid(),nbuffers/2); 
	// REMARK: these two streams do I/O while
	//         _functor is being computed (overlapping for free)
	
	_ExtIterator _cur = _begin - _begin.block_offset();
	
	// leave part of the block before _begin untouched (e.g. copy)
	for( ;_cur != _begin;_cur++)
	{
		typename _ExtIterator::value_type tmp;
		in >> tmp;
		out << tmp;
	}
	
	// apply _functor to the range [_begin,_end)
	for( ;_cur != _end;_cur++)
	{
		typename _ExtIterator::value_type tmp;
		in >> tmp;
		_functor(tmp);
		out << tmp;
	}
	
	// leave part of the block after _end untouched
	if(_end.block_offset())
	{
		_ExtIterator _last_block_end = _end - _end.block_offset() + _ExtIterator::block_type::size;
		for( ;_cur != _last_block_end; _cur++)
		{
			typename _ExtIterator::value_type tmp;
			in >> tmp;
			out << tmp;
		}
	}	
	
	return _functor;
}

//! \brief External equivalent of std::find
//! \remark The implementation exploits \c \<stxxl\> buffered streams (computation and I/O overlapped)
//! \param _begin object of model of \c ext_random_access_iterator concept
//! \param _end object of model of \c ext_random_access_iterator concept
//! \param _value value that is equality comparable to the _ExtIterator's value type
//! \param nbuffers number of buffers for internal use (should be at least 2*D )
//! \return first iterator \c i in the range [_begin,_end) such that *( \c i ) == \c _value, if no
//!         such exists then \c _end
template <typename _ExtIterator,typename _EqualityComparable>
_ExtIterator find(_ExtIterator _begin, _ExtIterator _end, const _EqualityComparable & _value,int nbuffers)
{
	typedef buf_istream<typename _ExtIterator::block_type,typename _ExtIterator::bids_container_iterator> buf_istream_type;
	
	_begin.flush(); // flush container
	
	// create prefetching stream, 
	buf_istream_type in(_begin.bid(),_end.bid() + ((_end.block_offset())?1:0),nbuffers); 
	
	_ExtIterator _cur = _begin - _begin.block_offset();
	
	// skip part of the block before _begin untouched
	for( ;_cur != _begin;_cur++)
		in++;
	
	// search in the the range [_begin,_end)
	for( ;_cur != _end;_cur++)
	{
		typename _ExtIterator::value_type tmp;
		in >> tmp;
		if(tmp == _value)
			return _cur;
	}
	
	return _cur;
}
__STXXL_END_NAMESPACE

#endif
