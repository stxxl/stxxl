/***************************************************************************
 *  lib/mng/block_manager.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/common/types.h>
#include <stxxl/bits/io/create_file.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/bits/mng/config.h>
#include <stxxl/bits/mng/disk_block_allocator.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/bits/io/disk_queues.h>

#include <cstddef>
#include <fstream>
#include <string>

namespace stxxl {

class io_error;

block_manager::block_manager()
{
    config* config = config::get_instance();

    // initialize config (may read config files now)
    config->check_initialized();

    // allocate block_allocators_
    ndisks = config->disks_number();
    block_allocators_.resize(ndisks);
    disk_files_.resize(ndisks);

    uint64 total_size = 0;

    for (unsigned i = 0; i < ndisks; ++i)
    {
        disk_config& cfg = config->disk(i);

        // assign queues in order of disks.
        if (cfg.queue == file::DEFAULT_QUEUE)
            cfg.queue = i;

        try
        {
            disk_files_[i] = create_file(cfg, file::CREAT | file::RDWR, i);

            STXXL_MSG("Disk '" << cfg.path << "' is allocated, space: " <<
                      (cfg.size) / (1024 * 1024) <<
                      " MiB, I/O implementation: " << cfg.fileio_string());
        }
        catch (io_error&)
        {
            STXXL_MSG("Error allocating disk '" << cfg.path << "', space: " <<
                      (cfg.size) / (1024 * 1024) <<
                      " MiB, I/O implementation: " << cfg.fileio_string());
            throw;
        }

        total_size += cfg.size;

        // create queue for the file.
        disk_queues::get_instance()->make_queue(disk_files_[i].get());

        block_allocators_[i] = new disk_block_allocator(disk_files_[i].get(), cfg);
    }

    if (ndisks > 1)
    {
        STXXL_MSG("In total " << ndisks << " disks are allocated, space: " <<
                  (total_size / (1024 * 1024)) <<
                  " MiB");
    }

    current_allocation_ = 0;
    total_allocation_ = 0;
    maximum_allocation_ = 0;
}

block_manager::~block_manager()
{
    STXXL_VERBOSE1("Block manager destructor");
    for (size_t i = ndisks; i > 0; )
    {
        --i;
        delete block_allocators_[i];
        disk_files_[i].reset();
    }
}

uint64 block_manager::get_total_bytes() const
{
    uint64 total = 0;

    for (unsigned i = 0; i < ndisks; ++i)
        total += block_allocators_[i]->get_total_bytes();

    return total;
}

uint64 block_manager::get_free_bytes() const
{
    uint64 total = 0;

    for (unsigned i = 0; i < ndisks; ++i)
        total += block_allocators_[i]->get_free_bytes();

    return total;
}

} // namespace stxxl
// vim: et:ts=4:sw=4
