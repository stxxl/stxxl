/***************************************************************************
 *  lib/io/fileperblock_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2008, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/common/aligned_alloc.h>
#include <stxxl/bits/common/counting_ptr.h>
#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/io/disk_queued_file.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/fileperblock_file.h>
#include <stxxl/bits/io/mmap_file.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/serving_request.h>
#include <stxxl/bits/io/syscall_file.h>
#include <stxxl/bits/io/wincall_file.h>
#include <stxxl/bits/io/wincall_file.h>
#include <stxxl/bits/unused.h>
#include <stxxl/bits/verbose.h>

#include "ufs_platform.h"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

namespace stxxl {

template <class base_file_type>
fileperblock_file<base_file_type>::fileperblock_file(
    const std::string& filename_prefix,
    int mode,
    int queue_id,
    int allocator_id,
    unsigned int device_id)
    : file(device_id),
      disk_queued_file(queue_id, allocator_id),
      filename_prefix_(filename_prefix),
      mode_(mode),
      current_size_(0)
{ }

template <class base_file_type>
fileperblock_file<base_file_type>::~fileperblock_file()
{
    if (lock_file_)
        lock_file_->close_remove();
}

template <class base_file_type>
std::string fileperblock_file<base_file_type>::filename_for_block(offset_type offset)
{
    std::ostringstream name;
    //enough for 1 billion blocks
    name << filename_prefix_ << "_fpb_" << std::setw(20) << std::setfill('0') << offset;
    return name.str();
}

template <class base_file_type>
void fileperblock_file<base_file_type>::serve(
    void* buffer, offset_type offset,
    size_type bytes, request::read_or_write op)
{
    base_file_type base_file(filename_for_block(offset), mode_, get_queue_id(),
                             NO_ALLOCATOR, DEFAULT_DEVICE_ID, file_stats_);
    base_file.set_size(bytes);
    base_file.serve(buffer, 0, bytes, op);
}

template <class base_file_type>
void fileperblock_file<base_file_type>::lock()
{
    if (!lock_file_)
    {
        lock_file_ = make_counting<base_file_type>(
            filename_prefix_ + "_fpb_lock", mode_, get_queue_id());

        //create lock file and fill it with one page, an empty file cannot be locked
        const int page_size = STXXL_BLOCK_ALIGN;
        void* one_page = aligned_alloc<STXXL_BLOCK_ALIGN>(page_size);
#if STXXL_WITH_VALGRIND
        memset(one_page, 0, page_size);
#endif
        lock_file_->set_size(page_size);
        request_ptr r = lock_file_->awrite(one_page, 0, page_size);
        r->wait();
        aligned_dealloc<STXXL_BLOCK_ALIGN>(one_page);
    }
    lock_file_->lock();
}

template <class base_file_type>
void fileperblock_file<base_file_type>::discard(offset_type offset, offset_type length)
{
    STXXL_UNUSED(length);
#ifdef STXXL_FILEPERBLOCK_NO_DELETE
    if (::truncate(filename_for_block(offset).c_str(), 0) != 0)
        STXXL_ERRMSG("truncate() error on path=" << filename_for_block(offset) << " error=" << strerror(errno));
#else
    if (::remove(filename_for_block(offset).c_str()) != 0)
        STXXL_ERRMSG("remove() error on path=" << filename_for_block(offset) << " error=" << strerror(errno));
#endif

    STXXL_VERBOSE2("discard " << offset << " + " << length);
}

template <class base_file_type>
void fileperblock_file<base_file_type>::export_files(offset_type offset, offset_type length, std::string filename)
{
    std::string original(filename_for_block(offset));
    filename.insert(0, original.substr(0, original.find_last_of("/") + 1));
    if (::remove(filename.c_str()) != 0)
        STXXL_ERRMSG("remove() error on path=" << filename << " error=" << strerror(errno));

    if (::rename(original.c_str(), filename.c_str()) != 0)
        STXXL_ERRMSG("rename() error on path=" << filename << " to=" << original << " error=" << strerror(errno));

#if !STXXL_WINDOWS
    //TODO: implement on Windows
    if (::truncate(filename.c_str(), as_signed(length)) != 0) {
        STXXL_THROW_ERRNO(io_error, "Error doing truncate()");
    }
#else
    STXXL_UNUSED(length);
#endif
}

template <class base_file_type>
const char* fileperblock_file<base_file_type>::io_type() const
{
    return "fileperblock";
}

////////////////////////////////////////////////////////////////////////////

template class fileperblock_file<syscall_file>;

#if STXXL_HAVE_MMAP_FILE
template class fileperblock_file<mmap_file>;
#endif

#if STXXL_HAVE_WINCALL_FILE
template class fileperblock_file<wincall_file>;
#endif

} // namespace stxxl

/******************************************************************************/
