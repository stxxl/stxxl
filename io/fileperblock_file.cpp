/***************************************************************************
 *  io/fileperblock_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <sstream>
#include <iomanip>
#include <stxxl/bits/io/fileperblock_file.h>
#include <stxxl/bits/io/syscall_file.h>
#include <stxxl/bits/io/mmap_file.h>
#include <stxxl/bits/io/boostfd_file.h>
#include <stxxl/bits/io/wincall_file.h>
#include <stxxl/bits/io/request_impl_basic.h>
#include <stxxl/bits/common/aligned_alloc.h>

__STXXL_BEGIN_NAMESPACE

template <class base_file_type>
fileperblock_file<base_file_type>::fileperblock_file(
    const std::string & filename_prefix,
    int mode,
    int disk)
    : file_request_basic(disk), filename_prefix(filename_prefix), mode(mode),
      lock_file_created(false), lock_file(filename_prefix + "_fpb_lock", mode, disk)
{ }

template <class base_file_type>
fileperblock_file<base_file_type>::~fileperblock_file()
{
    if (lock_file_created)
        ::remove((filename_prefix + "_fpb_lock").c_str());
}

template <class base_file_type>
std::string fileperblock_file<base_file_type>::filename_for_block(unsigned_type offset)
{
    std::ostringstream name;
    //enough for 1 billion blocks
    name << filename_prefix << "_fpb_" << std::setw(20) << std::setfill('0') << offset;
    return name.str();
}

template <class base_file_type>
void fileperblock_file<base_file_type>::serve(const request * req) throw (io_error)
{
    assert(req->get_file() == this);

    base_file_type base_file(filename_for_block(req->get_offset()), mode, get_id());
    base_file.set_size(req->get_size());

    request_ptr derived = new request_impl_basic(default_completion_handler(), &base_file, req->get_buffer(), 0, req->get_size(), req->get_type());
    request_ptr dummy = derived;
    derived->serve();
}

template <class base_file_type>
void fileperblock_file<base_file_type>::lock()
{
    if (!lock_file_created)
    {
        //create lock file and fill it with one page, an empty file cannot be locked
        const int page_size = BLOCK_ALIGN;
        void * one_page = aligned_alloc<BLOCK_ALIGN>(page_size);
        lock_file.set_size(page_size);
        request_ptr r = lock_file.awrite(one_page, 0, page_size, default_completion_handler());
        r->wait();
        aligned_dealloc<BLOCK_ALIGN>(one_page);
        lock_file_created = true;
    }
    lock_file.lock();
}

template <class base_file_type>
void fileperblock_file<base_file_type>::delete_region(offset_type offset, unsigned_type length)
{
    UNUSED(length);
    ::remove(filename_for_block(offset).c_str());
    STXXL_VERBOSE0("delete_region " << offset << " + " << length);
}

template <class base_file_type>
void fileperblock_file<base_file_type>::export_files(offset_type offset, offset_type length, std::string filename)
{
    std::string original(filename_for_block(offset));
    filename.insert(0, original.substr(0, original.find_last_of("/") + 1));
    ::remove(filename.c_str());
    ::rename(original.c_str(), filename.c_str());
#ifndef BOOST_MSVC
    //TODO: implement on Windows
    ::truncate(filename.c_str(), length);
#endif
}

template <class base_file_type>
const char * fileperblock_file<base_file_type>::io_type() const
{
    return "fileperblock";
}

////////////////////////////////////////////////////////////////////////////

template class fileperblock_file<syscall_file>;

#ifndef BOOST_MSVC

// mmap call does not exist in Windows
template class fileperblock_file<mmap_file>;

#else

template class fileperblock_file<wincall_file>;

#endif

#ifdef STXXL_BOOST_CONFIG
template class fileperblock_file<boostfd_file>;
#endif

__STXXL_END_NAMESPACE
