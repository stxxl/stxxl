/***************************************************************************
 *  include/stxxl/bits/io/wfs_file_base.h
 *
 *  Windows file system file base
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009, 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_WFS_FILE_BASE_HEADER
#define STXXL_IO_WFS_FILE_BASE_HEADER

#include <stxxl/bits/config.h>

#if STXXL_WINDOWS

#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request.h>

#include <string>

namespace stxxl {

//! \addtogroup fileimpl
//! \{

//! Base for Windows file system implementations.
class wfs_file_base : public virtual file
{
protected:
    using HANDLE = void*;

    mutex fd_mutex_;       // sequentialize function calls involving file_des_
    HANDLE file_des_;      // file descriptor
    int mode_;             // open mode
    const std::string filename_;
    offset_type bytes_per_sector_;
    bool locked_;
    wfs_file_base(const std::string& filename, int mode);
    offset_type _size();
    void close();

public:
    ~wfs_file_base();
    offset_type size();
    void set_size(offset_type newsize);
    void lock();
    const char * io_type() const;
    void close_remove();
};

//! \}

} // namespace stxxl

#endif // STXXL_WINDOWS

#endif // !STXXL_IO_WFS_FILE_BASE_HEADER
