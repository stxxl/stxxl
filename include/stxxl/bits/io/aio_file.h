/***************************************************************************
 *  include/stxxl/bits/io/aio_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_AIO_FILE_HEADER
#define STXXL_AIO_FILE_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifndef BOOST_MSVC
// libaio does not exist on Windows

#include <stxxl/bits/io/ufs_file_base.h>
#include <stxxl/bits/io/aio_queue.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on the POSIX interface for asynchronous I/O
class aio_file : public ufs_file_base
{
	friend class aio_request;

private:
	int physical_device_id, allocator_id;

public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resqueue_ided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) queue_identifier
    aio_file(
        const std::string& filename,
        int mode, int physical_device_id = DEFAULT_QUEUE, int allocator_id = NO_ALLOCATOR) : ufs_file_base(filename, mode), physical_device_id(physical_device_id), allocator_id(allocator_id)
    {
    }
    void serve(const request* req) throw (io_error);
    request_ptr aread(void* buffer, offset_type pos, size_type bytes,
                              const completion_handler& on_cmpl);
    request_ptr awrite(void* buffer, offset_type pos, size_type bytes,
                               const completion_handler& on_cmpl);
    const char* io_type() const;
    int get_queue_id() const { assert(false); return NO_QUEUE; }
    int get_allocator_id() const { return allocator_id; }
    virtual int get_physical_device_id() const
    {
    	return physical_device_id;
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // #ifndef BOOST_MSVC

#endif // !STXXL_AIO_FILE_HEADER
