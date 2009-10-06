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

#include <aio.h>
#include <stxxl/bits/io/aio_file.h>
#include <stxxl/bits/io/request_impl_basic.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

//! \brief Request for aio_file
class aio_request : public request_impl_basic
{
    template <class base_file_type>
    friend class fileperblock_file;

    aiocb64 cb;

public:
    aio_request(
        const completion_handler & on_cmpl,
        file * f,
        void * buf,
        offset_type off,
        size_type b,
        request_type t) :
        	request_impl_basic(on_cmpl, f, buf, off, b, t)
        	{

        	}

    bool post()
    {
    	aio_file* af = dynamic_cast<aio_file*>(file_);
    	cb.aio_fildes = af->get_file_des();
    	cb.aio_offset = offset;
    	cb.aio_buf = buffer;
    	cb.aio_nbytes = bytes;
    	cb.aio_reqprio = 0;
    	//cb.aio_sigevent = SIGEV_NONE;

    	int success;
    	if (type == READ)
    		success = aio_read64(&cb);
    	else
    		success = aio_write64(&cb);

    	return success != EAGAIN;
    }

    aiocb64* get_cb()
    {
    	return &cb;
    }

public:
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_AIO_REQUEST_HEADER
