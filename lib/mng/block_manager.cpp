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

#include <stxxl/bits/mng/block_manager.h>

#include <stxxl/bits/common/types.h>
#include <stxxl/bits/io/create_file.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/mng/config.h>
#include <stxxl/bits/mng/disk_block_allocator.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/bits/io/disk_queues.h>

#include <cstddef>
#include <string>

namespace stxxl {

class io_error;

block_manager::block_manager()
{
    config* config = config::get_instance();

    // initialize config (may read config files now)
    config->check_initialized();

    // allocate block_allocators_
    ndisks_ = config->disks_number();
    block_allocators_.resize(ndisks_);
    disk_files_.resize(ndisks_);

    uint64_t total_size = 0;

    for (size_t i = 0; i < ndisks_; ++i)
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

    if (ndisks_ > 1)
    {
        STXXL_MSG("In total " << ndisks_ << " disks are allocated, space: " <<
                  (total_size / (1024 * 1024)) <<
                  " MiB");
    }
}

block_manager::~block_manager()
{
    STXXL_VERBOSE1("Block manager destructor");
    for (size_t i = ndisks_; i > 0; )
    {
        --i;
        delete block_allocators_[i];
        disk_files_[i].reset();
    }
}

uint64_t block_manager::total_bytes() const
{
    std::unique_lock<std::mutex> lock(mutex_);

    uint64_t total = 0;

    for (size_t i = 0; i < ndisks_; ++i)
        total += block_allocators_[i]->total_bytes();

    return total;
}

uint64_t block_manager::free_bytes() const
{
    std::unique_lock<std::mutex> lock(mutex_);

    uint64_t total = 0;

    for (size_t i = 0; i < ndisks_; ++i)
        total += block_allocators_[i]->free_bytes();

    return total;
}

uint64_t block_manager::total_allocation() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return total_allocation_;
}

uint64_t block_manager::current_allocation() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return current_allocation_;
}

uint64_t block_manager::maximum_allocation() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return maximum_allocation_;
}

} // namespace stxxl
// vim: et:ts=4:sw=4
