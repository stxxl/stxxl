/***************************************************************************
 *  include/stxxl/bits/io/file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2008, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_FILE_HEADER
#define STXXL_IO_FILE_HEADER

#include <stxxl/bits/config.h>
#include <stxxl/bits/io/iostats.h>

#include <stxxl/bits/common/counting_ptr.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/libstxxl.h>
#include <stxxl/bits/unused.h>
#include <stxxl/bits/verbose.h>

#if defined(__linux__)
 #define STXXL_CHECK_BLOCK_ALIGNING
#endif

#include <cassert>
#include <ostream>
#include <string>

namespace stxxl {

//! \addtogroup iolayer
//! \{

//! \defgroup fileimpl File I/O Implementations
//! Implementations of \c stxxl::file for various file access methods and
//! operating systems.
//! \{

//! Defines interface of file.
//!
//! It is a base class for different implementations that might
//! base on various file systems or even remote storage interfaces
class file : public reference_count
{
public:
    //! the offset of a request, also the size of the file
    using offset_type = request::offset_type;
    //! the size of a request
    using size_type = request::size_type;

    //! Definition of acceptable file open modes.  Various open modes in a file
    //!system must be converted to this set of acceptable modes
    enum open_mode
    {
        //! only reading of the file is allowed
        RDONLY = 1,
        //! only writing of the file is allowed
        WRONLY = 2,
        //! read and write of the file are allowed
        RDWR = 4,
        //! in case file does not exist no error occurs and file is newly
        //! created
        CREAT = 8,
        //! I/Os proceed bypassing file system buffers, i.e. unbuffered I/O.
        //! Tries to open with appropriate flags, if fails print warning and
        //! open normally.
        DIRECT = 16,
        //! once file is opened its length becomes zero
        TRUNC = 32,
        //! open the file with O_SYNC | O_DSYNC | O_RSYNC flags set
        SYNC = 64,
        //! do not acquire an exclusive lock by default
        NO_LOCK = 128,
        //! implies DIRECT, fail if opening with DIRECT flag does not work.
        REQUIRE_DIRECT = 256
    };

    static const int DEFAULT_QUEUE = -1;
    static const int DEFAULT_LINUXAIO_QUEUE = -2;
    static const int NO_ALLOCATOR = -1;
    static const unsigned int DEFAULT_DEVICE_ID = (unsigned int)(-1);

    //! Construct a new file, usually called by a subclass.
    explicit file(unsigned int device_id = DEFAULT_DEVICE_ID)
        : device_id_(device_id),
          file_stats_(stats::get_instance()->create_file_stats(device_id))
    { }

    //! non-copyable: delete copy-constructor
    file(const file&) = delete;
    //! non-copyable: delete assignment operator
    file& operator = (const file&) = delete;
    //! move-constructor: default
    file(file&&) = default;
    //! move-assignment operator: default
    file& operator = (file&&) = default;

    //! Schedules an asynchronous read request to the file.
    //! \param buffer pointer to memory buffer to read into
    //! \param pos file position to start read from
    //! \param bytes number of bytes to transfer
    //! \param on_complete I/O completion handler
    //! \return \c request_ptr request object, which can be used to track the
    //! status of the operation

    virtual request_ptr aread(
        void* buffer, offset_type pos, size_type bytes,
        const completion_handler& on_complete = completion_handler()) = 0;

    //! Schedules an asynchronous write request to the file.
    //! \param buffer pointer to memory buffer to write from
    //! \param pos starting file position to write
    //! \param bytes number of bytes to transfer
    //! \param on_complete I/O completion handler
    //! \return \c request_ptr request object, which can be used to track the
    //! status of the operation

    virtual request_ptr awrite(
        void* buffer, offset_type pos, size_type bytes,
        const completion_handler& on_complete = completion_handler()) = 0;

    virtual void serve(void* buffer, offset_type offset, size_type bytes,
                       request::read_or_write op) = 0;

    //! Changes the size of the file.
    //! \param newsize new file size
    virtual void set_size(offset_type newsize) = 0;

    //! Returns size of the file.
    //! \return file size in bytes
    virtual offset_type size() = 0;

    //! Returns the identifier of the file's queue number.
    //! \remark Files allocated on the same physical device usually share the
    //! same queue, unless there is a common queue (e.g. with linuxaio).
    virtual int get_queue_id() const = 0;

    //! Returns the file's parallel disk block allocator number
    virtual int get_allocator_id() const = 0;

    //! Locks file for reading and writing (acquires a lock in the file system).
    virtual void lock() = 0;

    //! Discard a region of the file (mark it unused).
    //! Some specialized file types may need to know freed regions
    virtual void discard(offset_type offset, offset_type size)
    {
        STXXL_UNUSED(offset);
        STXXL_UNUSED(size);
    }

    virtual void export_files(offset_type offset, offset_type length,
                              std::string prefix)
    {
        STXXL_UNUSED(offset);
        STXXL_UNUSED(length);
        STXXL_UNUSED(prefix);
    }

    //! close and remove file
    virtual void close_remove() { }

    virtual ~file()
    {
        size_t nr = get_request_nref();
        if (nr != 0)
            STXXL_ERRMSG("stxxl::file is being deleted while there are "
                         "still " << nr << " (unfinished) requests "
                         "referencing it");
    }

    //! Identifies the type of I/O implementation.
    //! \return pointer to null terminated string of characters, containing the
    //! name of I/O implementation
    virtual const char * io_type() const = 0;

protected:
    //! The file's physical device id (e.g. used for prefetching sequence
    //! calculation)
    unsigned int device_id_;

    //! pointer to file's stats inside of iostats. Because the stats can live
    //! longer than the file, the iostats keeps ownership.
    file_stats* file_stats_;

public:
    //! Returns the file's physical device id
    unsigned int get_device_id() const
    {
        return device_id_;
    }

    file_stats * get_file_stats() const
    {
        return file_stats_;
    }

protected:
    //! count the number of requests referencing this file
    reference_count m_request_ref;

public:
    //! increment referenced requests
    void add_request_ref()
    {
        m_request_ref.inc_reference();
    }

    //! decrement referenced requests
    void delete_request_ref()
    {
        m_request_ref.dec_reference();
    }

    //! return number of referenced requests
    size_t get_request_nref()
    {
        return m_request_ref.get_reference_count();
    }

public:
    //! \name Static Functions for Platform Abstraction
    //! \{

    //! unlink path from filesystem
    static int unlink(const char* path);

    //! truncate a path to given length. Use this only if you dont have a
    //! fileio-specific object, which provides truncate().
    static int truncate(const char* path, external_size_type length);

    //! \}
};

//! \}

//! \defgroup reqlayer I/O Requests and Queues
//! Encapsulation of an I/O request, queues for requests and threads to process
//! them.
//! \{
//! \}

//! \}

} // namespace stxxl

#endif // !STXXL_IO_FILE_HEADER
// vim: et:ts=4:sw=4
