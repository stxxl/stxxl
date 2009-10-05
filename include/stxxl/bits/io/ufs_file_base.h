/***************************************************************************
 *  include/stxxl/bits/io/ufs_file_base.h
 *
 *  UNIX file system file base
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_UFSFILEBASE_HEADER
#define STXXL_UFSFILEBASE_HEADER

#include <stxxl/bits/io/async_file.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Base for UNIX file system implementations
class ufs_file_base : public async_file
{
protected:
    mutex fd_mutex;        // sequentialize function calls involving file_des
    int file_des;          // file descriptor
    int mode_;             // open mode
    ufs_file_base(const std::string & filename, int mode, int disk);
    offset_type _size();

public:
    ~ufs_file_base();
    offset_type size();
    void set_size(offset_type newsize);
    void lock();
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_UFSFILEBASE_HEADER
