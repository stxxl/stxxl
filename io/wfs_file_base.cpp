/***************************************************************************
 *  io/wfs_file_base.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/wfs_file_base.h>

#ifdef BOOST_MSVC

__STXXL_BEGIN_NAMESPACE


const char * wfs_file_base::io_type() const
{
    return "wfs_base";
}

static HANDLE open_file_impl(const std::string & filename, int mode)
{
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreationDisposition = 0;
    DWORD dwFlagsAndAttributes = 0;

    if (mode & file::RDONLY)
    {
        dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
        dwDesiredAccess |= GENERIC_READ;
    }

    if (mode & file::WRONLY)
    {
        dwDesiredAccess |= GENERIC_WRITE;
    }

    if (mode & file::RDWR)
    {
        dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
    }

    if (mode & file::CREAT)
    {
        // ignored
    }

    if (mode & file::TRUNC)
    {
        dwCreationDisposition |= TRUNCATE_EXISTING;
    }
    else
    {
        dwCreationDisposition |= OPEN_ALWAYS;
    }

#ifndef STXXL_DIRECT_IO_OFF
    if (mode & file::DIRECT)
    {
        dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
        // TODO: try also FILE_FLAG_WRITE_THROUGH option ?
    }
#endif

    if (mode & file::SYNC)
    {
        // ignored
    }

    HANDLE file_des = ::CreateFile(filename.c_str(), dwDesiredAccess, dwShareMode, NULL,
                                   dwCreationDisposition, dwFlagsAndAttributes, NULL);

    if (file_des == INVALID_HANDLE_VALUE)
        stxxl_win_lasterror_exit("CreateFile  filename=" << filename, io_error);

    return file_des;
}

wfs_file_base::wfs_file_base(
    const std::string & filename,
    int mode) : file_des(INVALID_HANDLE_VALUE), mode_(mode), filename(filename)
{
    file_des = open_file_impl(filename, mode);
    if (!(mode_ & RDONLY) && (mode & DIRECT))
    {
        char buf[32768], * part;
        if (!GetFullPathName(filename.c_str(), sizeof(buf), buf, &part))
        {
            STXXL_ERRMSG("wfs_file_base::wfs_file_base(): GetFullPathName() error for file " << filename);
            bytes_per_sector = 512;
        }
        else
        {
            part[0] = char();
            DWORD bytes_per_sector_;
            if (!GetDiskFreeSpace(buf, NULL, &bytes_per_sector_, NULL, NULL))
            {
                STXXL_ERRMSG("wfs_file_base::wfs_file_base(): GetDiskFreeSpace() error for path " << buf);
                bytes_per_sector = 512;
            }
            else
                bytes_per_sector = bytes_per_sector_;
        }
    }
}

wfs_file_base::~wfs_file_base()
{
    close();
}

void wfs_file_base::close()
{
    scoped_mutex_lock fd_lock(fd_mutex);

    if (file_des == INVALID_HANDLE_VALUE)
        return;

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

    if (!(mode_ & RDONLY))
    {
        LARGE_INTEGER desired_pos;
        desired_pos.QuadPart = newsize;

        bool direct_with_bad_size = (mode_ & file::DIRECT) && (newsize % bytes_per_sector);
        if (direct_with_bad_size)
        {
            if (!CloseHandle(file_des))
                stxxl_win_lasterror_exit("closing file (call of ::CloseHandle from set_size) ", io_error);

            file_des = INVALID_HANDLE_VALUE;
            file_des = open_file_impl(filename, WRONLY);
        }

        if (!SetFilePointerEx(file_des, desired_pos, NULL, FILE_BEGIN))
            stxxl_win_lasterror_exit("SetFilePointerEx in wfs_file_base::set_size(..) oldsize=" << cur_size <<
                                     " newsize=" << newsize << " ", io_error);

        if (!SetEndOfFile(file_des))
            stxxl_win_lasterror_exit("SetEndOfFile oldsize=" << cur_size <<
                                     " newsize=" << newsize << " ", io_error);

        if (direct_with_bad_size)
        {
            if (!CloseHandle(file_des))
                stxxl_win_lasterror_exit("closing file (call of ::CloseHandle from set_size) ", io_error);

            file_des = INVALID_HANDLE_VALUE;
            file_des = open_file_impl(filename, mode_ & ~TRUNC);
        }
    }
}

void wfs_file_base::remove()
{
    close();
    ::DeleteFile(filename.c_str());
}

__STXXL_END_NAMESPACE

#endif // #ifdef BOOST_MSVC
// vim: et:ts=4:sw=4
