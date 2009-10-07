/***************************************************************************
 *  io/aio_request.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iomanip>
#include <stxxl/bits/io/aio_request.h>
#include <stxxl/bits/io/file.h>


__STXXL_BEGIN_NAMESPACE

const char* aio_request::io_type() const
{
    return file_->io_type();
}

void aio_request::completed_callback(sigval_t sigval)
{
	static_cast<aio_request*>(sigval.sival_ptr)->completed();
	request_ptr r(static_cast<aio_request*>(sigval.sival_ptr));
	aio_file::q.complete_request(r);
}

bool aio_request::post()
{
	aio_file* af = dynamic_cast<aio_file*>(file_);
	cb.aio_fildes = af->file_des;
	cb.aio_offset = offset;
	cb.aio_buf = buffer;
	cb.aio_nbytes = bytes;
	cb.aio_reqprio = 0;
	cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
	cb.aio_sigevent.sigev_notify_function = completed_callback;
	cb.aio_sigevent.sigev_notify_attributes = NULL;
	cb.aio_sigevent.sigev_value.sival_ptr = this;

	int success;
	if (type == READ)
		success = aio_read64(&cb);
	else
		success = aio_write64(&cb);

	return success != EAGAIN;
}

bool aio_request::cancel()
{
    return aio_cancel64(cb.aio_fildes, &cb) == AIO_CANCELED;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
