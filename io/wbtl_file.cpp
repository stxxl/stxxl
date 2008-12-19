/***************************************************************************
 *  io/wbtl_file.cpp
 *
 *  a write-buffered-translation-layer pseudo file
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iomanip>
#include <stxxl/bits/io/wbtl_file.h>
#include <stxxl/bits/io/io.h>
#include <stxxl/bits/common/debug.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/aligned_alloc>

#ifndef STXXL_VERBOSE_WBTL
#define STXXL_VERBOSE_WBTL STXXL_VERBOSE0
#endif


__STXXL_BEGIN_NAMESPACE


// FIXME: make configurable: storage file type, write_block_size
wbtl_file::wbtl_file(
    const std::string & filename,
    int mode,
    int disk) :
        file_request_basic(disk), sz(0), write_block_size(32 * 1024 * 1024),
        free_bytes(0), curbuf(1), curpos(write_block_size)
{
    storage = new syscall_file(filename, mode);
    write_buffer[0] = static_cast<char *>(stxxl::aligned_alloc<BLOCK_ALIGN>(write_block_size));
    write_buffer[1] = static_cast<char *>(stxxl::aligned_alloc<BLOCK_ALIGN>(write_block_size));
    buffer_address[0] = offset_type(-1);
    buffer_address[1] = offset_type(-1);
}

wbtl_file::~wbtl_file()
{
    stxxl::aligned_dealloc<BLOCK_ALIGN>(write_buffer[1]);
    stxxl::aligned_dealloc<BLOCK_ALIGN>(write_buffer[0]);
    delete storage;
}

void wbtl_file::serve(const stxxl::request* req) throw(io_error)
{
    assert(req->get_file() == this);
    offset_type offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_type bytes = req->get_size();
    request::request_type type = req->get_type();

    if (type == request::READ)
    {
        //stats::scoped_read_timer read_timer(size());
        sread(buffer, offset, bytes);
    }
    else
    {
        //stats::scoped_write_timer write_timer(size());
        swrite(buffer, offset, bytes);
    }
}

void wbtl_file::lock()
{
    storage->lock();
}

wbtl_file::offset_type wbtl_file::size()
{
    return sz;
}

void wbtl_file::set_size(offset_type newsize)
{
    assert(sz <= newsize); // may not shrink
    if (sz < newsize) {
        _add_free_region(sz, newsize - sz);
        storage->set_size(newsize);
        sz = newsize;
        assert(sz == storage->size());
    }
}

request_ptr wbtl_file::aread(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    if (address_mapping.find(pos) == address_mapping.end()) {
        STXXL_THROW2(io_error, "wbtl_aread of unmapped memory");
    }

    return file_request_basic::aread(buffer, pos, bytes, on_cmpl);
}

#define FMT_A_S(_addr_,_size_) "0x" << std::hex << std::setfill('0') << std::setw(8) << (_addr_) << "/0x" << std::setw(8) << (_size_)
#define FMT_A_C(_addr_,_size_) "0x" << std::setw(8) << (_addr_) << "(" << std::dec << (_size_) << ")"
#define FMT_A(_addr_) "0x" << std::setw(8) << (_addr_)

// logical address
void wbtl_file::delete_region(offset_type offset, size_type size)
{
    sortseq::iterator physical = address_mapping.find(offset);
    STXXL_VERBOSE_WBTL("wbtl:delreg  l" << FMT_A_S(offset, size) << " @    p" << FMT_A(physical != address_mapping.end() ? physical->second : 0xffffffff));
    if (physical == address_mapping.end()) {
        // could be OK if the block was never written ...
        STXXL_ERRMSG("delete_region: mapping not found: " << FMT_A_S(offset, size) << " ==> " << "???");
    } else {
        size_type physical_offset = physical->second;
        address_mapping.erase(physical);
        _add_free_region(physical_offset, size);
    }
}

// physical address
void wbtl_file::_add_free_region(offset_type offset, offset_type size)
{
    STXXL_VERBOSE_WBTL("wbtl:addfre  p" << FMT_A_S(offset, size) << " F <= f" << FMT_A_C(free_bytes, free_space.size()));
    size_type region_pos = offset;
    size_type region_size = size;
    if (!free_space.empty())
    {
        sortseq::iterator succ = free_space.upper_bound(region_pos);
        sortseq::iterator pred = succ;
        pred--;
        check_corruption(region_pos, region_size, pred, succ);
        if (succ == free_space.end())
        {
            if (pred == free_space.end())
            {
                //dump();
                assert(pred != free_space.end());
            }
            if ((*pred).first + (*pred).second == region_pos)
            {
                // coalesce with predecessor
                region_size += (*pred).second;
                region_pos = (*pred).first;
                free_space.erase(pred);
            }
        }
        else
        {
            if (free_space.size() > 1)
            {
                bool succ_is_not_the_first = (succ != free_space.begin());
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space.erase(succ);
                }
                if (succ_is_not_the_first)
                {
                    if (pred == free_space.end())
                    {
                        //dump();
                        assert(pred != free_space.end());
                    }
                    if ((*pred).first + (*pred).second == region_pos)
                    {
                        // coalesce with predecessor
                        region_size += (*pred).second;
                        region_pos = (*pred).first;
                        free_space.erase(pred);
                    }
                }
            }
            else
            {
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space.erase(succ);
                }
            }
        }
    }

    free_space[region_pos] = region_size;
    free_bytes += size;
    STXXL_VERBOSE_WBTL("wbtl:free    p" << FMT_A_S(region_pos, region_size) << " F => f" << FMT_A_C(free_bytes, free_space.size()));
}

void wbtl_file::sread(void * buffer, offset_type offset, size_type bytes)
{
    int cached = -1;
    // map logical to physical address
    sortseq::iterator physical = address_mapping.find(offset);
    if (physical == address_mapping.end()) {
        STXXL_ERRMSG("wbtl_read: mapping not found: " << FMT_A_S(offset, bytes) << " ==> " << "???");
        //STXXL_THROW2(io_error, "wbtl_read of unmapped memory");
    }
    size_type physical_offset = physical->second;

    if (buffer_address[curbuf] <= physical_offset &&
        physical_offset < buffer_address[curbuf] + write_block_size)
    {
        // block is in current write buffer
        assert(physical_offset + bytes <= buffer_address[curbuf] + write_block_size);
        memcpy(buffer, write_buffer[curbuf] + (physical_offset - buffer_address[curbuf]), bytes);
        cached = curbuf;
    }
    else if (buffer_address[1 - curbuf] <= physical_offset &&
             physical_offset < buffer_address[1 - curbuf] + write_block_size)
    {
        // block is in previous write buffer
        assert(physical_offset + bytes <= buffer_address[1 - curbuf] + write_block_size);
        memcpy(buffer, write_buffer[1 - curbuf] + (physical_offset - buffer_address[1 - curbuf]), bytes);
        cached = curbuf;
    }
    else
    {
        // block is not cached
        request_ptr req = storage->aread(buffer, physical_offset, bytes, default_completion_handler());
        req->wait();
    }
    STXXL_VERBOSE_WBTL("wbtl:sread   l" << FMT_A_S(offset, bytes) << " @    p" << FMT_A(physical != address_mapping.end() ? physical_offset : 0xffffffff) << " " << std::dec << cached);
}

void wbtl_file::swrite(void * buffer, offset_type offset, size_type bytes)
{
    // is the block already mapped?
    sortseq::iterator physical = address_mapping.find(offset);
    STXXL_VERBOSE_WBTL("wbtl:swrite  l" << FMT_A_S(offset, bytes) << " @ <= p" <<
                       FMT_A_C(physical != address_mapping.end() ? physical->second : 0xffffffff, address_mapping.size()));
    if (physical != address_mapping.end()) {
        // FIXME: special case if we can replace it in the current writing block
        delete_region(offset, bytes);
    }

    if (bytes > write_block_size - curpos)
    {
        // not enough space in the current write buffer

        if (buffer_address[curbuf] != offset_type(-1)) {
            STXXL_VERBOSE_WBTL("wbtl:w2disk  p" << FMT_A_S(buffer_address[curbuf], write_block_size));

            // mark remaining part as free
            if (curpos < write_block_size)
                _add_free_region(buffer_address[curbuf] + curpos, write_block_size - curpos);
            
            if (backend_request.get()) {
                backend_request->wait();
            }

            backend_request = storage->awrite(write_buffer[curbuf], buffer_address[curbuf], write_block_size, default_completion_handler());
        }

        curbuf = 1 - curbuf;

        buffer_address[curbuf] = get_next_write_block();
        curpos = 0;
    }
    assert(bytes <= write_block_size - curpos);

    // write block into buffer
    memcpy(write_buffer[curbuf] + curpos, buffer, bytes);
    
    address_mapping[offset] = buffer_address[curbuf] + curpos;
    STXXL_VERBOSE_WBTL("wbtl:swrite  l" << FMT_A_S(offset, bytes) << " @ => p" << FMT_A_C(buffer_address[curbuf] + curpos, address_mapping.size()));
    curpos += bytes;
}

wbtl_file::offset_type wbtl_file::get_next_write_block()
{
    sortseq::iterator space =
        std::find_if(free_space.begin(), free_space.end(),
                     bind2nd(FirstFit(), write_block_size));

    if (space != free_space.end())
    {
        stxxl::int64 region_pos = (*space).first;
        stxxl::int64 region_size = (*space).second;
        free_space.erase(space);
        if (region_size > write_block_size)
            free_space[region_pos + write_block_size] = region_size - write_block_size;

        free_bytes -= write_block_size;

        STXXL_VERBOSE_WBTL("wbtl:nextwb  p" << FMT_A_S(region_pos, write_block_size) << " F    f" << FMT_A_C(free_bytes, free_space.size()));
        return region_pos;
    }

    STXXL_ERRMSG("OutOfSpace, probably fragmented");

    return -1;
}

void wbtl_file::check_corruption(offset_type region_pos, offset_type region_size,
                      sortseq::iterator pred, sortseq::iterator succ)
{
    if (pred != free_space.end())
    {
        if (pred->first <= region_pos && pred->first + pred->second > region_pos)
        {
            STXXL_THROW(bad_ext_alloc, "DiskAllocator::check_corruption", "Error: double deallocation of external memory " <<
                        "System info: P " << pred->first << " " << pred->second << " " << region_pos);
        }
    }
    if (succ != free_space.end())
    {
        if (region_pos <= succ->first && region_pos + region_size > succ->first)
        {
            STXXL_THROW(bad_ext_alloc, "DiskAllocator::check_corruption", "Error: double deallocation of external memory "
                 << "System info: S " << region_pos << " " << region_size << " " << succ->first);
        }
    }
}

const char * wbtl_file::io_type() const
{
    return "wbtl";
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
