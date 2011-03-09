/***************************************************************************
 *  include/stxxl/bits/io/serving_request.h
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

#include <stxxl/bits/io/request_with_state.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

//! \brief Basic implementation of request
class serving_request : public request_state_impl_basic
{
    template <class base_file_type>
    friend class fileperblock_file;

public:
    serving_request(
        const completion_handler & on_cmpl,
        file * f,
        void * buf,
        offset_type off,
        size_type b,
        request_type t);

protected:
    void serve();
    void completed();

public:
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_REQUEST_IMPL_BASIC_HEADER
