/***************************************************************************
 *  include/stxxl/bits/io/syscall_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SYSCALL_FILE_HEADER
#define STXXL_SYSCALL_FILE_HEADER

#include <stxxl/bits/io/ufs_file_base.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on UNIX syscalls
class syscall_file : public ufs_file_base
{
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    syscall_file(
        const std::string & filename,
        int mode,
        int disk = -1) : ufs_file_base(filename, mode, disk)
    { }
    void serve(const request * req) throw(io_error);
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_SYSCALL_FILE_HEADER
