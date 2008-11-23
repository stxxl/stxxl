/***************************************************************************
 *  include/stxxl/bits/io/request_impl_basic.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_REQUEST_IMPL_BASIC_HEADER
#define STXXL_IO_REQUEST_IMPL_BASIC_HEADER

#include <stxxl/bits/io/request_state_impl_basic.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

//! \brief Basic implementation of request
class request_impl_basic : public request_state_impl_basic
{
    //friend class file;

public:
    request_impl_basic(
        completion_handler on_cmpl,
        file * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t);

protected:
    void serve();

public:
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_REQUEST_IMPL_BASIC_HEADER
