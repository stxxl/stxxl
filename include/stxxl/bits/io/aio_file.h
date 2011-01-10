/***************************************************************************
 *  include/stxxl/bits/io/aio_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
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
// kernel aio interface does not exist on Windows
 #define STXXL_HAVE_AIO_FILE 1
#else
 #define STXXL_HAVE_AIO_FILE 0
#endif


#if STXXL_HAVE_AIO_FILE

#include <stxxl/bits/io/ufs_file_base.h>
#include <stxxl/bits/io/aio_queue.h>


__STXXL_BEGIN_NAMESPACE

class aio_queue;

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of \c file based on the Linux kernel interface for asynchronous I/O
class aio_file : public ufs_file_base
{
    friend class aio_request;

private:
    int physical_device_id, allocator_id;
    aio_queue * queue;

    aio_queue * get_queue() { return queue; }

public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    aio_file(
        const std::string & filename,
        int mode, int physical_device_id = DEFAULT_QUEUE, int allocator_id = NO_ALLOCATOR) :
        ufs_file_base(filename, mode), physical_device_id(physical_device_id), allocator_id(allocator_id)
    { }

    void serve(const request * req) throw (io_error);
    request_ptr aread(void * buffer, offset_type pos, size_type bytes,
                      const completion_handler & on_cmpl);
    request_ptr awrite(void * buffer, offset_type pos, size_type bytes,
                       const completion_handler & on_cmpl);
    const char * io_type() const;

    int get_queue_id() const
    {
        return physical_device_id;
    }

    int get_allocator_id() const
    {
        return allocator_id;
    }

    int get_physical_device_id() const
    {
        return physical_device_id;
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // #if STXXL_HAVE_AIO_FILE

#endif // !STXXL_AIO_FILE_HEADER
// vim: et:ts=4:sw=4
