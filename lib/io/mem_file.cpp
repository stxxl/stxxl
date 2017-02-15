/***************************************************************************
 *  lib/io/mem_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstring>
#include <limits>
#include <cassert>

#include <stxxl/bits/io/mem_file.h>
#include <stxxl/bits/io/iostats.h>

namespace stxxl {

void mem_file::serve(void* buffer, offset_type offset, size_type bytes,
                     request::read_or_write type)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (type == request::READ)
    {
        stats::scoped_read_timer read_timer(bytes);
        memcpy(buffer, ptr_ + offset, bytes);
    }
    else
    {
        stats::scoped_write_timer write_timer(bytes);
        memcpy(ptr_ + offset, buffer, bytes);
    }
}

const char* mem_file::io_type() const
{
    return "memory";
}

mem_file::~mem_file()
{
    free(ptr_);
    ptr_ = NULL;
}

void mem_file::lock()
{
    // nothing to do
}

file::offset_type mem_file::size()
{
    return size_;
}

void mem_file::set_size(offset_type newsize)
{
    std::unique_lock<std::mutex> lock(mutex_);
    assert(newsize <= std::numeric_limits<offset_type>::max());

    ptr_ = (char*)realloc(ptr_, (size_t)newsize);
    size_ = newsize;
}

void mem_file::discard(offset_type offset, offset_type size)
{
    std::unique_lock<std::mutex> lock(mutex_);
#ifndef STXXL_MEMFILE_DONT_CLEAR_FREED_MEMORY
    // overwrite the freed region with uninitialized memory
    STXXL_VERBOSE("discard at " << offset << " len " << size);
    void* uninitialized = malloc(STXXL_BLOCK_ALIGN);
    while (size >= STXXL_BLOCK_ALIGN) {
        memcpy(ptr_ + offset, uninitialized, STXXL_BLOCK_ALIGN);
        offset += STXXL_BLOCK_ALIGN;
        size -= STXXL_BLOCK_ALIGN;
    }
    assert(size <= std::numeric_limits<offset_type>::max());
    if (size > 0)
        memcpy(ptr_ + offset, uninitialized, (size_t)size);
    free(uninitialized);
#else
    STXXL_UNUSED(offset);
    STXXL_UNUSED(size);
#endif
}

} // namespace stxxl

/******************************************************************************/
