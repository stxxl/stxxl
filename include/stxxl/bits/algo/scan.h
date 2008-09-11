/***************************************************************************
 *  include/stxxl/bits/algo/scan.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SCAN_HEADER
#define STXXL_SCAN_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/mng/buf_istream.h>
#include <stxxl/bits/mng/buf_ostream.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlalgo
//! \{

//! \brief External equivalent of std::for_each
//! \remark The implementation exploits \c \<stxxl\> buffered streams (computation and I/O overlapped)
//! \param _begin object of model of \c ext_random_access_iterator concept
//! \param _end object of model of \c ext_random_access_iterator concept
//! \param _functor function object of model of \c std::UnaryFunction concept
//! \param nbuffers number of buffers (blocks) for internal use (should be at least 2*D )
//! \return function object \c _functor after it has been applied to the each element of the given range
//!
//! \warning nested stxxl::for_each are not supported
template <typename _ExtIterator, typename _UnaryFunction>
_UnaryFunction for_each(_ExtIterator _begin, _ExtIterator _end, _UnaryFunction _functor, int_type nbuffers)
{
    typedef buf_istream<typename _ExtIterator::block_type, typename _ExtIterator::bids_container_iterator> buf_istream_type;

    _begin.flush();     // flush container

    // create prefetching stream,
    buf_istream_type in(_begin.bid(), _end.bid() + ((_end.block_offset()) ? 1 : 0), nbuffers);

    _ExtIterator _cur = _begin - _begin.block_offset();

    // leave part of the block before _begin untouched (e.g. copy)
    for ( ; _cur != _begin; ++_cur)
    {
        typename _ExtIterator::value_type tmp;
        in >> tmp;
    }

    // apply _functor to the range [_begin,_end)
    for ( ; _cur != _end; ++_cur)
    {
        typename _ExtIterator::value_type tmp;
        in >> tmp;
        _functor(tmp);
    }

    // leave part of the block after _end untouched
    if (_end.block_offset())
    {
        _ExtIterator _last_block_end = _end - _end.block_offset() + _ExtIterator::block_type::size;
        for ( ; _cur != _last_block_end; ++_cur)
        {
            typename _ExtIterator::value_type tmp;
            in >> tmp;
        }
    }

    return _functor;
}


//! \brief External equivalent of std::for_each (mutating)
//! \remark The implementation exploits \c \<stxxl\> buffered streams (computation and I/O overlapped)
//! \param _begin object of model of \c ext_random_access_iterator concept
//! \param _end object of model of \c ext_random_access_iterator concept
//! \param _functor
//! \param nbuffers number of buffers (blocks) for internal use (should be at least 2*D )
//! \return function object \c _functor after it has been applied to the each element of the given range
//!
//! \warning nested stxxl::for_each_m are not supported
template <typename _ExtIterator, typename _UnaryFunction>
_UnaryFunction for_each_m(_ExtIterator _begin, _ExtIterator _end, _UnaryFunction _functor, int_type nbuffers)
{
    typedef buf_istream<typename _ExtIterator::block_type, typename _ExtIterator::bids_container_iterator> buf_istream_type;
    typedef buf_ostream<typename _ExtIterator::block_type, typename _ExtIterator::bids_container_iterator> buf_ostream_type;

    _begin.flush();     // flush container

    // create prefetching stream,
    buf_istream_type in(_begin.bid(), _end.bid() + ((_end.block_offset()) ? 1 : 0), nbuffers / 2);
    // create buffered write stream for blocks
    buf_ostream_type out(_begin.bid(), nbuffers / 2);
    // REMARK: these two streams do I/O while
    //         _functor is being computed (overlapping for free)

    _ExtIterator _cur = _begin - _begin.block_offset();

    // leave part of the block before _begin untouched (e.g. copy)
    for ( ; _cur != _begin; ++_cur)
    {
        typename _ExtIterator::value_type tmp;
        in >> tmp;
        out << tmp;
    }

    // apply _functor to the range [_begin,_end)
    for ( ; _cur != _end; ++_cur)
    {
        typename _ExtIterator::value_type tmp;
        in >> tmp;
        _functor(tmp);
        out << tmp;
    }

    // leave part of the block after _end untouched
    if (_end.block_offset())
    {
        _ExtIterator _last_block_end = _end - _end.block_offset() + _ExtIterator::block_type::size;
        for ( ; _cur != _last_block_end; ++_cur)
        {
            typename _ExtIterator::value_type tmp;
            in >> tmp;
            out << tmp;
        }
    }

    return _functor;
}


//! \brief External equivalent of std::generate
//! \remark The implementation exploits \c \<stxxl\> buffered streams (computation and I/O overlapped)
//! \param _begin object of model of \c ext_random_access_iterator concept
//! \param _end object of model of \c ext_random_access_iterator concept
//! \param _generator function object of model of \c std::Generator concept
//! \param nbuffers number of buffers (blocks) for internal use (should be at least 2*D )
template <typename _ExtIterator, typename _Generator>
void generate(_ExtIterator _begin, _ExtIterator _end, _Generator _generator, int_type nbuffers)
{
    typedef typename _ExtIterator::block_type block_type;
    typedef buf_ostream<block_type, typename _ExtIterator::bids_container_iterator> buf_ostream_type;


    while (_begin.block_offset())    //  go to the beginning of the block
    //  of the external vector
    {
        if (_begin == _end)
            return;

        *_begin = _generator();
        ++_begin;
    }

    _begin.flush();     // flush container

    // create buffered write stream for blocks
    buf_ostream_type outstream(_begin.bid(), nbuffers);

    assert(_begin.block_offset() == 0);

    while (_end != _begin)
    {
        if (_begin.block_offset() == 0)
            _begin.block_externally_updated();

        *outstream = _generator();
        ++_begin;
        ++outstream;
    }

    typename _ExtIterator::const_iterator out = _begin;

    while (out.block_offset())    // filling the rest of the block
    {
        *outstream = *out;
        ++out;
        ++outstream;
    }
    _begin.flush();
}

//! \brief External equivalent of std::find
//! \remark The implementation exploits \c \<stxxl\> buffered streams (computation and I/O overlapped)
//! \param _begin object of model of \c ext_random_access_iterator concept
//! \param _end object of model of \c ext_random_access_iterator concept
//! \param _value value that is equality comparable to the _ExtIterator's value type
//! \param nbuffers number of buffers (blocks) for internal use (should be at least 2*D )
//! \return first iterator \c i in the range [_begin,_end) such that *( \c i ) == \c _value, if no
//!         such exists then \c _end
template <typename _ExtIterator, typename _EqualityComparable>
_ExtIterator find(_ExtIterator _begin, _ExtIterator _end, const _EqualityComparable & _value, int_type nbuffers)
{
    typedef buf_istream<typename _ExtIterator::block_type, typename _ExtIterator::bids_container_iterator> buf_istream_type;

    _begin.flush();     // flush container

    // create prefetching stream,
    buf_istream_type in(_begin.bid(), _end.bid() + ((_end.block_offset()) ? 1 : 0), nbuffers);

    _ExtIterator _cur = _begin - _begin.block_offset();

    // skip part of the block before _begin untouched
    for ( ; _cur != _begin; ++_cur)
        ++in;


    // search in the the range [_begin,_end)
    for ( ; _cur != _end; ++_cur)
    {
        typename _ExtIterator::value_type tmp;
        in >> tmp;
        if (tmp == _value)
            return _cur;
    }

    return _cur;
}

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_SCAN_HEADER
