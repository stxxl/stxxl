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
#include <stxxl/bits/io/request_impl_basic.h>

__STXXL_BEGIN_NAMESPACE

template<class base_file_type>
fileperblock_file<base_file_type>::fileperblock_file(
    const std::string & filename_prefix,
    int mode,
    int disk)
        : file_request_basic(disk), filename_prefix(filename_prefix), mode(mode), disk(disk)
{
}

template<class base_file_type>
std::string fileperblock_file<base_file_type>::filename_for_block(unsigned_type offset)
{
    std::ostringstream name;
    //enough for 1 billion blocks
    name << filename_prefix << "_fpb_" << std::setw(20) << std::setfill('0') << offset;
    return name.str();
}

template<class base_file_type>
void fileperblock_file<base_file_type>::serve(const request * req) throw(io_error)
{
    assert(req->get_file() == this);

    base_file_type base_file(filename_for_block(req->get_offset()), mode, disk);
    base_file.set_size(req->get_size());

    request_impl_basic derived(req->get_on_complete(), &base_file, req->get_buffer(), 0, req->get_size(), req->get_type());

    base_file.serve(&derived);

    derived.completed();
}

template<class base_file_type>
void fileperblock_file<base_file_type>::lock()
{
}

template<class base_file_type>
void fileperblock_file<base_file_type>::delete_region(offset_type offset, unsigned_type length)
{
    UNUSED(length);
    ::remove(filename_for_block(offset).c_str());
    STXXL_VERBOSE0("delete_region " << offset << " + " << length)
}

template<class base_file_type>
void fileperblock_file<base_file_type>::export_files(offset_type offset, offset_type length, std::string filename)
{
    std::string original(filename_for_block(offset));
    filename.insert(0, original.substr(0, original.find_last_of("/") + 1));
    ::remove(filename.c_str());
    ::rename(original.c_str(), filename.c_str());
    ::truncate(filename.c_str(), length);
}

template<class base_file_type>
const char * fileperblock_file<base_file_type>::io_type() const
{
    return "fileperblock";
}

////////////////////////////////////////////////////////////////////////////

template class fileperblock_file<syscall_file>;

template class fileperblock_file<mmap_file>;

__STXXL_END_NAMESPACE
