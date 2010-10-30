/***************************************************************************
 *  mng/mng.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/common/debug.h>


__STXXL_BEGIN_NAMESPACE

void DiskAllocator::dump()
{
    int64 total = 0;
    sortseq::const_iterator cur = free_space.begin();
    STXXL_ERRMSG("Free regions dump:");
    for ( ; cur != free_space.end(); ++cur)
    {
        STXXL_ERRMSG("Free chunk: begin: " << (cur->first) << " size: " << (cur->second));
        total += cur->second;
    }
    STXXL_ERRMSG("Total bytes: " << total);
}


block_manager::block_manager()
{
    debugmon::get_instance();
    config * cfg = config::get_instance();

    ndisks = cfg->disks_number();
    disk_allocators = new DiskAllocator *[ndisks];
    disk_files = new file *[ndisks];

    for (unsigned i = 0; i < ndisks; ++i)
    {
        disk_files[i] = create_file(cfg->disk_io_impl(i),
                                            cfg->disk_path(i),
                                            file::CREAT | file::RDWR | file::DIRECT,
                                            i,  // physical_device_id
                                            i); // allocator_id
        disk_allocators[i] = new DiskAllocator(disk_files[i], cfg->disk_size(i));
    }
}

block_manager::~block_manager()
{
    STXXL_VERBOSE1("Block manager destructor");
    for (unsigned i = ndisks; i > 0; )
    {
        --i;
        delete disk_allocators[i];
        delete disk_files[i];
    }
    delete[] disk_allocators;
    delete[] disk_files;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
