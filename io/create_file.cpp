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

#include <stxxl/bits/io/create_file.h>
#include <stxxl/bits/io/io.h>


__STXXL_BEGIN_NAMESPACE

file * create_file(const std::string & io_impl,
                   const std::string & filename,
                   int options, int physical_device_id, int allocator_id)
{
    if (io_impl == "syscall")
    {
        ufs_file_base * result = new syscall_file(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_syscall")
    {
        fileperblock_file<syscall_file> * result = new fileperblock_file<syscall_file>(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
#if STXXL_HAVE_MMAP_FILE
    else if (io_impl == "mmap")
    {
        ufs_file_base * result = new mmap_file(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_mmap")
    {
        fileperblock_file<mmap_file> * result = new fileperblock_file<mmap_file>(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
#endif
#if STXXL_HAVE_SIMDISK_FILE
    else if (io_impl == "simdisk")
    {
        ufs_file_base * result = new sim_disk_file(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
#endif
#if STXXL_HAVE_WINCALL_FILE
    else if (io_impl == "wincall")
    {
        wfs_file_base * result = new wincall_file(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_wincall")
    {
        fileperblock_file<wincall_file> * result = new fileperblock_file<wincall_file>(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
#endif
#if STXXL_HAVE_BOOSTFD_FILE
    else if (io_impl == "boostfd")
    {
        boostfd_file * result = new boostfd_file(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_boostfd")
    {
        fileperblock_file<boostfd_file> * result = new fileperblock_file<boostfd_file>(filename, options, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
#endif
    else if (io_impl == "memory")
    {
        mem_file * result = new mem_file(physical_device_id, allocator_id);
        result->lock();
        return result;
    }
#if STXXL_HAVE_WBTL_FILE
    else if (io_impl == "wbtl")
    {
        ufs_file_base * backend = new syscall_file(filename, options, -1, -1); // FIXME: ID
        wbtl_file * result = new stxxl::wbtl_file(backend, 16 * 1024 * 1024, 2, physical_device_id, allocator_id);
        result->lock();
        return result;
    }
#endif

    STXXL_THROW(std::runtime_error, "FileCreator::create", "Unsupported disk I/O implementation " <<
                io_impl << " .");

    return NULL;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
