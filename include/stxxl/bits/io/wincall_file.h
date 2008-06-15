/***************************************************************************
 *  include/stxxl/bits/io/wincall_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_WINCALL_FILE_HEADER
#define STXXL_WINCALL_FILE_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif


#ifdef BOOST_MSVC

#include "stxxl/bits/io/wfs_file.h"
#include "stxxl/bits/common/debug.h"


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on Windows native I/O calls
class wincall_file : public wfs_file_base
{
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    wincall_file(
        const std::string & filename,
        int mode,
        int disk = -1);
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

//! \brief Implementation of request based on UNIX syscalls
class wincall_request : public wfs_request_base
{
    friend class wincall_file;

protected:
    wincall_request(
        wincall_file * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t,
        completion_handler on_cmpl);
    void serve ();

public:
    const char * io_type ();
};

//! \}

__STXXL_END_NAMESPACE

#endif // #ifdef BOOST_MSVC

#endif // !STXXL_WINCALL_FILE_HEADER
