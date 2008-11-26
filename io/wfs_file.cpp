/***************************************************************************
 *  io/wfs_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/wfs_file.h>

#ifdef BOOST_MSVC

__STXXL_BEGIN_NAMESPACE


const char * wfs_file_base::io_type() const
{
    return "wfs_base";
}

wfs_file_base::wfs_file_base(
    const std::string & filename,
    int mode,
    int disk) : file_request_basic(disk), file_des(INVALID_HANDLE_VALUE), mode_(mode)
{
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreationDisposition = 0;
    DWORD dwFlagsAndAttributes = 0;

 #ifndef STXXL_DIRECT_IO_OFF
    if (mode & DIRECT)
    {
        dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
        // TODO: try also FILE_FLAG_WRITE_THROUGH option ?
    }
 #endif

    if (mode & RDONLY)
    {
        dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
        dwDesiredAccess |= GENERIC_READ;
    }

    if (mode & WRONLY)
    {
        dwDesiredAccess |= GENERIC_WRITE;
    }

    if (mode & RDWR)
    {
        dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
    }

    if (mode & CREAT)
    {
        // ignored
    }


    if (mode & TRUNC)
    {
        dwCreationDisposition |= TRUNCATE_EXISTING;
    }
    else
    {
        dwCreationDisposition |= OPEN_ALWAYS;
    }

    file_des = ::CreateFile(filename.c_str(), dwDesiredAccess, dwShareMode, NULL,
                            dwCreationDisposition, dwFlagsAndAttributes, NULL);

    if (file_des == INVALID_HANDLE_VALUE)
        stxxl_win_lasterror_exit("CreateFile  filename=" << filename, io_error);
}

wfs_file_base::~wfs_file_base()
{
    scoped_mutex_lock fd_lock(fd_mutex);
    if (!CloseHandle(file_des))
        stxxl_win_lasterror_exit("closing file (call of ::CloseHandle) ", io_error);

        file_des = INVALID_HANDLE_VALUE;
}

void wfs_file_base::lock()
{
    scoped_mutex_lock fd_lock(fd_mutex);
    if (LockFile(file_des, 0, 0, 0xffffffff, 0xffffffff) == 0)
        stxxl_win_lasterror_exit("LockFile ", io_error);
}

file::offset_type wfs_file_base::_size()
{
    LARGE_INTEGER result;
    if (!GetFileSizeEx(file_des, &result))
        stxxl_win_lasterror_exit("GetFileSizeEx ", io_error);

        return result.QuadPart;
}

file::offset_type wfs_file_base::size()
{
    scoped_mutex_lock fd_lock(fd_mutex);
    return _size();
}

void wfs_file_base::set_size(offset_type newsize)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    offset_type cur_size = _size();

    LARGE_INTEGER desired_pos;
    desired_pos.QuadPart = newsize;

    if (!SetFilePointerEx(file_des, desired_pos, NULL, FILE_BEGIN))
        stxxl_win_lasterror_exit("SetFilePointerEx in wfs_file_base::set_size(..) oldsize=" << cur_size <<
                                 " newsize=" << newsize << " ", io_error);

        if (!SetEndOfFile(file_des))
            stxxl_win_lasterror_exit("SetEndOfFile oldsize=" << cur_size <<
                                     " newsize=" << newsize << " ", io_error);
}

__STXXL_END_NAMESPACE

#endif // #ifdef BOOST_MSVC
// vim: et:ts=4:sw=4
