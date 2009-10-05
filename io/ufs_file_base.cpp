/***************************************************************************
 *  io/ufs_file_base.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2005, 2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Ilja Andronov <sni4ok@yandex.ru>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/ufs_file_base.h>

#ifdef BOOST_MSVC
 #include <windows.h>
#else
 #include <unistd.h>
 #include <fcntl.h>
#endif


__STXXL_BEGIN_NAMESPACE


const char * ufs_file_base::io_type() const
{
    return "ufs_base";
}

ufs_file_base::ufs_file_base(
    const std::string & filename,
    int mode,
    int disk) : async_file(disk), file_des(-1), mode_(mode)
{
    int fmode = 0;

#ifndef STXXL_DIRECT_IO_OFF
 #ifndef BOOST_MSVC
    if (mode & DIRECT)
        fmode |= O_SYNC | O_RSYNC | O_DSYNC | O_DIRECT;
 #endif
#endif

    if (mode & RDONLY)
        fmode |= O_RDONLY;

    if (mode & WRONLY)
        fmode |= O_WRONLY;

    if (mode & RDWR)
        fmode |= O_RDWR;

    if (mode & CREAT)
        fmode |= O_CREAT;

    if (mode & TRUNC)
        fmode |= O_TRUNC;

#ifdef BOOST_MSVC
    fmode |= O_BINARY;                     // the default in MS is TEXT mode
#endif

#ifdef BOOST_MSVC
    const int flags = S_IREAD | S_IWRITE;
#else
    const int flags = S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP;
#endif

    if ((file_des = ::open(filename.c_str(), fmode, flags)) < 0)
        STXXL_THROW2(io_error, "Filedescriptor=" << file_des << " filename=" << filename << " fmode=" << fmode);
}

ufs_file_base::~ufs_file_base()
{
    scoped_mutex_lock fd_lock(fd_mutex);
    int res = ::close(file_des);

    // if successful, reset file descriptor
    if (res >= 0)
        file_des = -1;
    else
        stxxl_function_error(io_error);
}

void ufs_file_base::lock()
{
#ifdef BOOST_MSVC
    // not yet implemented
#else
    scoped_mutex_lock fd_lock(fd_mutex);
    struct flock lock_struct;
    lock_struct.l_type = F_RDLCK | F_WRLCK;
    lock_struct.l_whence = SEEK_SET;
    lock_struct.l_start = 0;
    lock_struct.l_len = 0; // lock all bytes
    if ((::fcntl(file_des, F_SETLK, &lock_struct)) < 0)
        STXXL_THROW2(io_error, "Filedescriptor=" << file_des);
#endif
}

file::offset_type ufs_file_base::_size()
{
    struct stat st;
    stxxl_check_ge_0(::fstat(file_des, &st), io_error);
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

    if (newsize > cur_size)
        stxxl_check_ge_0(::lseek(file_des, newsize - 1, SEEK_SET), io_error);
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
