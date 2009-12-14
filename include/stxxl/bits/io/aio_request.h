/***************************************************************************
 *  include/stxxl/bits/io/aio_request.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_AIO_REQUEST_HEADER
#define STXXL_IO_AIO_REQUEST_HEADER

#include <stxxl/bits/io/aio_file.h>

#if STXXL_HAVE_AIO_FILE

#include <linux/aio_abi.h>
#include <stxxl/bits/io/request_impl_basic.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

//! \brief Request for aio_file
class aio_request : public request_impl_basic
{
    template <class base_file_type>
    friend class fileperblock_file;

    iocb cb;	//control block
    iocb* get_cb() { return &cb; }	//must be initialized by post

    void fill_control_block();

public:
    aio_request(
        const completion_handler& on_cmpl,
        file* f,
        void* buf,
        offset_type off,
        size_type b,
        request_type t) :
        	request_impl_basic(on_cmpl, f, buf, off, b, t)
    {
       	assert(dynamic_cast<aio_file*>(file_));
    }

    bool post();
    bool cancel();
    void completed(bool canceled);
    void completed() { completed(false); }

    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // #if STXXL_HAVE_AIO_FILE

#endif // !STXXL_IO_AIO_REQUEST_HEADER
// vim: et:ts=4:sw=4
