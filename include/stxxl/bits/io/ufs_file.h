/***************************************************************************
 *  include/stxxl/bits/io/ufs_file.h
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

#include <stxxl/bits/io/iobase.h>
#include <stxxl/bits/io/basic_request_state.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Base for UNIX file system implementations
class ufs_file_base : public file
{
protected:
    int file_des;          // file descriptor
    int mode_;             // open mode
    ufs_file_base(const std::string & filename, int mode, int disk);

public:
    int get_file_des() const;
    ~ufs_file_base();
    stxxl::int64 size();
    void set_size(stxxl::int64 newsize);
    void lock();
};

//! \brief Base for UNIX file system implementations
class ufs_request_base : public basic_request_state
{
protected:
    ufs_request_base(
        ufs_file_base * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t,
        completion_handler on_cmpl);

public:
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_UFSFILEBASE_HEADER
