/***************************************************************************
 *  io/create_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2008, 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <sstream>

#include <stxxl/bits/io/create_file.h>
#include <stxxl/bits/io/io.h>


__STXXL_BEGIN_NAMESPACE

file * create_file(const std::string & io_impl,
                   const std::string & filename,
                   int options, int physical_device_id, int allocator_id)
{
    file* result;

    if (io_impl == "syscall")
        result = new syscall_file(filename, options, physical_device_id, allocator_id);
    else if (io_impl == "fileperblock_syscall")
        result = new fileperblock_file<syscall_file>(filename, options, physical_device_id, allocator_id);
#if STXXL_HAVE_MMAP_FILE
    else if (io_impl == "mmap")
        result = new mmap_file(filename, options, physical_device_id, allocator_id);
    else if (io_impl == "fileperblock_mmap")
        result = new fileperblock_file<mmap_file>(filename, options, physical_device_id, allocator_id);
#endif
#if STXXL_HAVE_AIO_FILE
    //aio can have the desired queue length immediately appended to "aio", e.g. "aio(5)"
    else if (io_impl.find("aio") == 0)
    {
        int desired_queue_length = 0;

        size_t opening = io_impl[3] == '(' ? 3 : std::string::npos, closing = io_impl.find_first_of(')');
        if (opening != std::string::npos && closing != std::string::npos && opening + 1 < closing)
        {
            std::istringstream input(io_impl.substr(opening + 1, closing - opening - 1));
            input >> desired_queue_length;
        }

        result = new aio_file(filename, options, physical_device_id, allocator_id, desired_queue_length);
    }
    else if (io_impl.find("fileperblock_aio") == 0)
        result = new fileperblock_file<aio_file>(filename, options, physical_device_id, allocator_id); //default queue length, does not matter anyway
    #endif
#if STXXL_HAVE_SIMDISK_FILE
    else if (io_impl == "simdisk")
    {
        options &= ~stxxl::file::DIRECT;  // clear the DIRECT flag, this file is supposed to be on tmpfs
        result = new sim_disk_file(filename, options, physical_device_id, allocator_id);
    }
#endif
#if STXXL_HAVE_WINCALL_FILE
    else if (io_impl == "wincall")
        result = new wincall_file(filename, options, physical_device_id, allocator_id);
    else if (io_impl == "fileperblock_wincall")
        result = new fileperblock_file<wincall_file>(filename, options, physical_device_id, allocator_id);
#endif
#if STXXL_HAVE_BOOSTFD_FILE
    else if (io_impl == "boostfd")
        result = new boostfd_file(filename, options, physical_device_id, allocator_id);
    else if (io_impl == "fileperblock_boostfd")
        result = new fileperblock_file<boostfd_file>(filename, options, physical_device_id, allocator_id);
#endif
    else if (io_impl == "memory")
        result = new mem_file(physical_device_id, allocator_id);
#if STXXL_HAVE_WBTL_FILE
    else if (io_impl == "wbtl")
    {
        ufs_file_base * backend = new syscall_file(filename, options, -1, -1); // FIXME: ID
        result = new stxxl::wbtl_file(backend, 16 * 1024 * 1024, 2, physical_device_id, allocator_id);
    }
#endif
    else
    {
        STXXL_THROW(std::runtime_error, "FileCreator::create", "Unsupported disk I/O implementation " <<
                    io_impl << " .");
    }

    result->lock();
    return result;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
