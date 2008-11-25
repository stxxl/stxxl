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

__STXXL_BEGIN_NAMESPACE

template<class base_file_type>
fileperblock_file<base_file_type>::fileperblock_file(
    const std::string & filename_prefix,
    int mode,
    int disk)
        : file(disk), filename_prefix(filename_prefix), mode(mode), disk(disk)
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
request_ptr fileperblock_file<base_file_type>::aread(
    void * buffer,
    stxxl::int64 offset,
    size_t length,
    completion_handler on_completion)
{
    request_ptr req = new fileperblock_request<base_file_type>(this,
                                          buffer, offset, length,
                                          request::READ, on_completion);
    return req;
}

template<class base_file_type>
request_ptr fileperblock_file<base_file_type>::awrite(
    void * buffer,
    stxxl::int64 offset,
    size_t length,
    completion_handler on_completion)
{
    request_ptr req = new fileperblock_request<base_file_type>(this,
                                          buffer, offset, length,
                                          request::WRITE, on_completion);
    return req;
}

template<class base_file_type>
void fileperblock_file<base_file_type>::delete_region(int64 offset, unsigned_type length)
{
    ::remove(filename_for_block(offset).c_str());
    STXXL_VERBOSE0("delete_region " << offset << " + " << length)
}

template<class base_file_type>
void fileperblock_file<base_file_type>::lock()
{
    // not yet implemented, maybe not needed
}

template<class base_file_type>
void fileperblock_file<base_file_type>::export_files(int64 offset, int64 length, std::string filename)
{
    std::string original(filename_for_block(offset));
    filename.insert(0, original.substr(0, original.find_last_of("/") + 1));
    ::remove(filename.c_str());
    ::rename(original.c_str(), filename.c_str());
    ::truncate(filename.c_str(), length);
}

////////////////////////////////////////////////////////////////////////////

template<class base_file_type>
fileperblock_request<base_file_type>::fileperblock_request(
    fileperblock_file<base_file_type>* f,
    void * buffer,
    stxxl::int64 offset,
    size_t length,
    typename base_file_type::request::request_type read_or_write,
    completion_handler on_completion)
   :
    request(on_completion, f, buffer, offset, length, read_or_write),
    base_file (new base_file_type(f->filename_for_block(offset), f->mode, f->disk))
{
    base_file->set_size(length);
    base_request = new request_ptr
       (read_or_write == request::READ ?
        base_file->aread(buffer, 0, length, on_completion) :
        base_file->awrite(buffer, 0, length, on_completion));
}

template<class base_file_type>
fileperblock_request<base_file_type>::~fileperblock_request()
{
    delete base_request;
    delete base_file;
}

template<class base_file_type>
void fileperblock_request<base_file_type>::serve()
{
    (*base_request)->serve();
}

template<class base_file_type>
const char * fileperblock_request<base_file_type>::io_type()
{
    return "fileperblock";
}

template<class base_file_type>
bool fileperblock_request<base_file_type>::add_waiter(onoff_switch * sw)
{
    return (*base_request)->add_waiter(sw);
}

template<class base_file_type>
void fileperblock_request<base_file_type>::delete_waiter(onoff_switch * sw)
{
    (*base_request)->delete_waiter(sw);
}

template<class base_file_type>
void fileperblock_request<base_file_type>::wait()
{
    (*base_request)->wait();
    delete base_file;
    base_file = NULL;
}

template<class base_file_type>
bool fileperblock_request<base_file_type>::poll()
{
    bool finished = (*base_request)->poll();
    if(finished) {
        delete base_file;
        base_file = NULL;
    }
    return finished;
}

////////////////////////////////////////////////////////////////////////////

template class fileperblock_file<syscall_file>;
template class fileperblock_request<syscall_file>;

template class fileperblock_file<mmap_file>;
template class fileperblock_request<mmap_file>;

__STXXL_END_NAMESPACE
