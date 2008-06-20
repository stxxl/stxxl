/***************************************************************************
 *  include/stxxl/bits/io/mmap_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MMAP_FILE_HEADER
#define STXXL_MMAP_FILE_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifndef BOOST_MSVC
// mmap call does not exist in Windows

#include <sys/mman.h>

#include <stxxl/bits/io/ufs_file.h>


__STXXL_BEGIN_NAMESPACE

//! \weakgroup fileimpl File implementations
//! \ingroup iolayer
//! Implementations of \c stxxl::file and \c stxxl::request
//! for various file access methods
//! \{

//! \brief Implementation of memory mapped access file
class mmap_file : public ufs_file_base
{
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    inline mmap_file(const std::string & filename, int mode, int disk = -1) :
        ufs_file_base(filename, mode, disk)
    { }
    request_ptr aread(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    request_ptr awrite(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
};

//! \brief Implementation of memory mapped access file request
class mmap_request : public ufs_request_base
{
    friend class mmap_file;

protected:
    inline mmap_request(mmap_file * f,
                        void * buf, stxxl::int64 off, size_t b,
                        request_type t,
                        completion_handler on_cmpl) :
        ufs_request_base(f, buf, off, b, t, on_cmpl)
    { }
    void serve();

public:
    inline const char * io_type()
    {
        return "mmap";
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // #ifndef BOOST_MSVC

#endif // !STXXL_MMAP_FILE_HEADER
