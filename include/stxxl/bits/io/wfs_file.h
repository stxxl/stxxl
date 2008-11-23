/***************************************************************************
 *  include/stxxl/bits/io/wfs_file.h
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

#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Base for Windows file system implementations
class wfs_file_base : public file
{
protected:
    HANDLE file_des;       // file descriptor
    int mode_;             // open mode
    wfs_file_base(const std::string & filename, int mode, int disk);

public:
    ~wfs_file_base();
    stxxl::int64 size();
    void set_size(stxxl::int64 newsize);
    void lock();
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // #ifdef BOOST_MSVC

#endif // !STXXL_WFSFILEBASE_HEADER
