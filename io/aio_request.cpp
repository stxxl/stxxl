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

#include <stxxl/bits/io/aio_request.h>

#if STXXL_HAVE_AIO_FILE

#include <iomanip>


__STXXL_BEGIN_NAMESPACE

const char* aio_request::io_type() const
{
    return file_->io_type();
}

void aio_request::completed(bool canceled)
{
	if (!canceled)
	{
		if (type == READ)
			stats::get_instance()->read_finished();
		else
			stats::get_instance()->write_finished();
	}
	else
	{
		if (type == READ)
			stats::get_instance()->read_canceled(bytes);
		else
			stats::get_instance()->write_canceled(bytes);
	}
	request_impl_basic::completed();
}

void aio_request::fill_control_block()
{
	aio_file* af = dynamic_cast<aio_file*>(file_);

	memset(&cb, 0, sizeof(cb));
	cb.data = this;
	cb.aio_fildes = af->file_des;
	cb.aio_lio_opcode = (type == READ) ? IO_CMD_PREAD : IO_CMD_PWRITE;
	cb.aio_reqprio = 0;
	cb.u.c.buf = buffer;
	cb.u.c.nbytes = bytes;
	cb.u.c.offset = offset;
}

bool aio_request::post()
{
	fill_control_block();
	iocb* cb_pointer = &cb;
	int success = io_submit(aio_queue::get_instance()->get_io_context(), 1, &cb_pointer);
	if (success == 1)
		stats::get_instance()->read_started(bytes);

	return success == 1;
}

bool aio_request::cancel()
{
	io_event event;
    int result = io_cancel(aio_queue::get_instance()->get_io_context(), &cb, &event);
    if (result == 0)
    	aio_queue::get_instance()->handle_events(&event, 1, true);
    std::cout << result << " " << EAGAIN << " " << ENOSYS << " " << EFAULT << " " << EINVAL << std::endl;
    return result == 0;
}

__STXXL_END_NAMESPACE

#endif // #if STXXL_HAVE_AIO_FILE
// vim: et:ts=4:sw=4
