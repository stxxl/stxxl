/***************************************************************************
 *  include/stxxl/bits/stream/choose.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM_CHOOSE_HEADER
#define STXXL_STREAM_CHOOSE_HEADER

#include <tuple>

namespace stxxl {

//! Stream package subnamespace.
namespace stream {

////////////////////////////////////////////////////////////////////////
//     CHOOSE                                                         //
////////////////////////////////////////////////////////////////////////

//! Creates stream from a tuple stream taking the which component of each tuple.
//!
//! \tparam Input type of the input tuple stream
//!
//! \remark Tuple stream is a stream which \c value_type is \c std::tuple .
template <class Input, int Which>
class choose
{
    Input& in;

    using tuple_type = typename Input::value_type;

public:
    //! Standard stream typedef.
    using value_type = typename std::tuple_element<Which, tuple_type>::type;

    //! Construction.
    explicit choose(Input& in_) : in(in_)
    { }

    //! Standard stream method.
    const value_type& operator * () const
    {
        return std::get<Which>(*in);
    }

    const value_type* operator -> () const
    {
        return &std::get<Which>(*in);
    }

    //! Standard stream method.
    choose& operator ++ ()
    {
        ++in;
        return *this;
    }

    //! Standard stream method.
    bool empty() const
    {
        return in.empty();
    }
};

} // namespace stream
} // namespace stxxl

#endif // !STXXL_STREAM_CHOOSE_HEADER
