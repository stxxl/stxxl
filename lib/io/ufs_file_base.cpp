/***************************************************************************
 *  io/ufs_file_base.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2005, 2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Ilja Andronov <sni4ok@yandex.ru>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/ufs_file_base.h>
#include <stxxl/bits/common/error_handling.h>

#ifdef BOOST_MSVC
 #include <windows.h>
#else
 #include <unistd.h>
 #include <fcntl.h>
#endif
#include <cstdio>


__STXXL_BEGIN_NAMESPACE


const char * ufs_file_base::io_type() const
{
    return "ufs_base";
}

ufs_file_base::ufs_file_base(
    const std::string & filename,
    int mode) : file_des(-1), mode_(mode), filename(filename)
{
    int flags = 0;

    if (mode & RDONLY)
    {
        flags |= O_RDONLY;
    }

    if (mode & WRONLY)
    {
        flags |= O_WRONLY;
    }

    if (mode & RDWR)
    {
        flags |= O_RDWR;
    }

    if (mode & CREAT)
    {
        flags |= O_CREAT;
    }

    if (mode & TRUNC)
    {
        flags |= O_TRUNC;
    }

#ifndef STXXL_DIRECT_IO_OFF
    if (mode & DIRECT)
    {
        flags |= O_DIRECT;
    }
#endif

    if (mode & SYNC)
    {
        flags |= O_RSYNC;
        flags |= O_DSYNC;
        flags |= O_SYNC;
    }

#ifdef BOOST_MSVC
    flags |= O_BINARY;                     // the default in MS is TEXT mode
#endif

#ifdef BOOST_MSVC
    const int perms = S_IREAD | S_IWRITE;
#else
    const int perms = S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP;
#endif

    if ((file_des = ::open(filename.c_str(), flags, perms)) < 0)
        STXXL_THROW2(io_error, "::open() rc=" << file_des << " path=" << filename << " flags=" << flags);

    if (!(mode & NO_LOCK))
    {
        lock();
    }
}

ufs_file_base::~ufs_file_base()
{
    close();
}

void ufs_file_base::close()
{
    scoped_mutex_lock fd_lock(fd_mutex);

    if (file_des == -1)
        return;

    if (::close(file_des) < 0)
        stxxl_function_error(io_error);

    file_des = -1;
}

void ufs_file_base::lock()
{
#ifdef BOOST_MSVC
    // not yet implemented
#else
    scoped_mutex_lock fd_lock(fd_mutex);
    struct flock lock_struct;
    lock_struct.l_type = (mode_ & RDONLY) ? F_RDLCK : F_RDLCK | F_WRLCK;
    lock_struct.l_whence = SEEK_SET;
    lock_struct.l_start = 0;
    lock_struct.l_len = 0; // lock all bytes
    if ((::fcntl(file_des, F_SETLK, &lock_struct)) < 0)
        STXXL_THROW2(io_error, "::fcntl(,F_SETLK,) path=" << filename << " fd=" << file_des);
#endif
}

file::offset_type ufs_file_base::_size()
{
#ifdef BOOST_MSVC
    struct _stat64 st;
    stxxl_check_ge_0(_fstat64(file_des, &st), io_error);
#else
    struct stat st;
    stxxl_check_ge_0(::fstat(file_des, &st), io_error);
#endif
    return st.st_size;
}

file::offset_type ufs_file_base::size()
{
    scoped_mutex_lock fd_lock(fd_mutex);
    return _size();
}

void ufs_file_base::set_size(offset_type newsize)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    offset_type cur_size = _size();

    if (!(mode_ & RDONLY))
    {
#ifdef BOOST_MSVC
        HANDLE hfile;
        stxxl_check_ge_0(hfile = (HANDLE) ::_get_osfhandle(file_des), io_error);

        LARGE_INTEGER desired_pos;
        desired_pos.QuadPart = newsize;

        if (!SetFilePointerEx(hfile, desired_pos, NULL, FILE_BEGIN))
            stxxl_win_lasterror_exit("SetFilePointerEx in ufs_file_base::set_size(..) oldsize=" << cur_size <<
                                     " newsize=" << newsize << " ", io_error);

        if (!SetEndOfFile(hfile))
            stxxl_win_lasterror_exit("SetEndOfFile oldsize=" << cur_size <<
                                     " newsize=" << newsize << " ", io_error);
#else
        stxxl_check_ge_0(::ftruncate(file_des, newsize), io_error);
#endif
    }

#ifndef BOOST_MSVC
    if (newsize > cur_size)
        stxxl_check_ge_0(::lseek(file_des, newsize - 1, SEEK_SET), io_error);
#endif
}

void ufs_file_base::remove()
{
    close();
    ::remove(filename.c_str());
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
