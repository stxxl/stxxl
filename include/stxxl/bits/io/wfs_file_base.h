/***************************************************************************
 *  include/stxxl/bits/io/wfs_file_base.h
 *
 *  Windows file system file base
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_WFSFILEBASE_HEADER
#define STXXL_WFSFILEBASE_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC

#include <stxxl/bits/io/file_request_basic.h>
#include <stxxl/bits/io/request.h>
#include <windows.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Base for Windows file system implementations
class wfs_file_base : public file_request_basic
{
protected:
    mutex fd_mutex;        // sequentialize function calls involving file_des
    HANDLE file_des;       // file descriptor
    int mode_;             // open mode
    wfs_file_base(const std::string & filename, int mode, int disk);
    offset_type _size();

public:
    ~wfs_file_base();
    offset_type size();
    void set_size(offset_type newsize);
    void lock();
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // #ifdef BOOST_MSVC

#endif // !STXXL_WFSFILEBASE_HEADER
