/***************************************************************************
 *  io/boostfd_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/boostfd_file.h>

#if STXXL_HAVE_BOOSTFD_FILE

#include <stxxl/bits/io/request_impl_basic.h>
#include <stxxl/bits/common/debug.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/version.hpp>


__STXXL_BEGIN_NAMESPACE


void boostfd_file::serve(const request * req) throw (io_error)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    assert(req->get_file() == this);
    offset_type offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_type bytes = req->get_size();
    request::request_type type = req->get_type();

    try
    {
        file_des.seek(offset, BOOST_IOS::beg);
    }
    catch (const std::exception & ex)
    {
        STXXL_THROW2(io_error,
                     "Error doing seek() in boostfd_request::serve()" <<
                     " offset=" << offset <<
                     " this=" << this <<
                     " buffer=" << buffer <<
                     " bytes=" << bytes <<
                     " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                     " : " << ex.what());
    }

    stats::scoped_read_write_timer read_write_timer(bytes, type == request::WRITE);

    if (type == request::READ)
    {
        STXXL_DEBUGMON_DO(io_started(buffer));

        try
        {
            file_des.read((char *)buffer, bytes);
        }
        catch (const std::exception & ex)
        {
            STXXL_THROW2(io_error,
                         "Error doing read() in boostfd_request::serve()" <<
                         " offset=" << offset <<
                         " this=" << this <<
                         " buffer=" << buffer <<
                         " bytes=" << bytes <<
                         " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                         " : " << ex.what());
        }

        STXXL_DEBUGMON_DO(io_finished(buffer));
    }
    else
    {
        STXXL_DEBUGMON_DO(io_started(buffer));

        try
        {
            file_des.write((char *)buffer, bytes);
        }
        catch (const std::exception & ex)
        {
            STXXL_THROW2(io_error,
                         "Error doing write() in boostfd_request::serve()" <<
                         " offset=" << offset <<
                         " this=" << this <<
                         " buffer=" << buffer <<
                         " bytes=" << bytes <<
                         " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                         " : " << ex.what());
        }

        STXXL_DEBUGMON_DO(io_finished(buffer));
    }
}

const char * boostfd_file::io_type() const
{
    return "boostfd";
}

boostfd_file::boostfd_file(
    const std::string & filename,
    int mode,
    int queue_id, int allocator_id) : disk_queued_file(queue_id, allocator_id), mode_(mode)
{
    BOOST_IOS::openmode boostfd_mode;

 #ifndef STXXL_DIRECT_IO_OFF
    if (mode & DIRECT)
    {
        // direct mode not supported in Boost
    }
 #endif

    if (mode & RDONLY)
    {
        boostfd_mode = BOOST_IOS::in;
    }

    if (mode & WRONLY)
    {
        boostfd_mode = BOOST_IOS::out;
    }

    if (mode & RDWR)
    {
        boostfd_mode = BOOST_IOS::out | BOOST_IOS::in;
    }

    const boost::filesystem::path fspath(filename,
                                         boost::filesystem::native);

    if (mode & TRUNC)
    {
        if (boost::filesystem::exists(fspath))
        {
            boost::filesystem::remove(fspath);
            boost::filesystem::ofstream f(fspath);
            f.close();
            assert(boost::filesystem::exists(fspath));
        }
    }

    if (mode & CREAT)
    {
        // need to be emulated:
        if (!boost::filesystem::exists(fspath))
        {
            boost::filesystem::ofstream f(fspath);
            f.close();
            assert(boost::filesystem::exists(fspath));
        }
    }

#if (BOOST_VERSION >= 104100)
    file_des.open(filename, boostfd_mode);	//also compiles with earlier Boost versions, but differs semantically
#else
    file_des.open(filename, boostfd_mode, boostfd_mode);
#endif
}

boostfd_file::~boostfd_file()
{
    scoped_mutex_lock fd_lock(fd_mutex);
    file_des.close();
}

inline file::offset_type boostfd_file::_size()
{
    return file_des.seek(0, BOOST_IOS::end);
}

file::offset_type boostfd_file::size()
{
    scoped_mutex_lock fd_lock(fd_mutex);
    return _size();
}

void boostfd_file::set_size(offset_type newsize)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    offset_type oldsize = _size();
    file_des.seek(newsize, BOOST_IOS::beg);
    file_des.seek(0, BOOST_IOS::beg); // not important ?
    assert(_size() >= oldsize);
}

void boostfd_file::lock()
{
    // FIXME: is there no locking possible/needed/... for boostfd?
}

__STXXL_END_NAMESPACE

#endif  // #if STXXL_HAVE_BOOSTFD_FILE
// vim: et:ts=4:sw=4
