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

__STXXL_BEGIN_NAMESPACE

template<class base_file_type>
fileperblock_file<base_file_type>::fileperblock_file(
    const std::string & filename,
    int mode,
    unsigned_type block_size,
    int disk)
        : file(disk), filename(filename), mode(mode), block_size(block_size), disk(disk)
{
}

template<class base_file_type>
std::string fileperblock_file<base_file_type>::file_name_for_block(unsigned_type offset)
{
    std::ostringstream name;
    name << filename << "_fpb_" << std::setw(10) << std::setfill('0') << (offset / block_size);
    return name.str();
}

template<class base_file_type>
request_ptr fileperblock_file<base_file_type>::aread(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new fileperblock_request<base_file_type>(this,
                                          buffer, pos, bytes,
                                          request::READ, on_cmpl);

    return req;
}

template<class base_file_type>
request_ptr fileperblock_file<base_file_type>::awrite(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new fileperblock_request<base_file_type>(this, buffer, pos, bytes,
                                          request::WRITE, on_cmpl);
    return req;
}

template<class base_file_type>
void fileperblock_file<base_file_type>::delete_region(int64 offset, unsigned_type size)
{
    if(size == block_size)
        ::remove(file_name_for_block(offset).c_str());
}

template<class base_file_type>
void fileperblock_file<base_file_type>::feasible(int64 offset, size_t size)
{
    assert((offset % block_size + size) <= block_size);
}

template<class base_file_type>
int64 fileperblock_file<base_file_type>::block_offset(int64 offset)
{
    return offset % block_size;
}

////////////////////////////////////////////////////////////////////////////

template<class base_file_type>
fileperblock_request<base_file_type>::fileperblock_request(
    fileperblock_file<base_file_type>* f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    typename base_file_type::request::request_type t,
    completion_handler on_cmpl)
   :
    request(on_cmpl, f, buf, off, b, t),
    base_file (new base_file_type(f->file_name_for_block(off), f->mode, f->disk)),
    base_request(t == request::READ ?
        base_file->aread(buf, f->block_offset(off), b, on_cmpl) :
        base_file->awrite(buf, f->block_offset(off), b, on_cmpl))
{
    f->feasible(off, b);
}

template<class base_file_type>
fileperblock_request<base_file_type>::~fileperblock_request()
{
    delete base_file;
}

template<class base_file_type>
void fileperblock_request<base_file_type>::serve()
{
    base_request->serve();
}

template<class base_file_type>
const char * fileperblock_request<base_file_type>::io_type()
{
    return "fileperblock";
}


template<class base_file_type>
bool fileperblock_request<base_file_type>::add_waiter(onoff_switch * sw)
{
    return base_request->add_waiter(sw);
}

template<class base_file_type>
void fileperblock_request<base_file_type>::delete_waiter(onoff_switch * sw)
{
    return base_request->delete_waiter(sw);
}

template<class base_file_type>
void fileperblock_request<base_file_type>::wait()
{
    base_request->wait();
}

template<class base_file_type>
bool fileperblock_request<base_file_type>::poll()
{
    return base_request->poll();
}

////////////////////////////////////////////////////////////////////////////

template class fileperblock_file<syscall_file>;
template class fileperblock_request<syscall_file>;

__STXXL_END_NAMESPACE
