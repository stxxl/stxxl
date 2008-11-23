/***************************************************************************
 *  include/stxxl/bits/io/mem_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MEM_FILE_HEADER
#define STXXL_MEM_FILE_HEADER

#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on new[] and memcpy
class mem_file : public file
{
    char * ptr;
    unsigned_type sz;

public:
    //! \brief constructs file object
    //! \param disk disk(file) identifier
    mem_file(
        int disk = -1);
    request_ptr aread(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    request_ptr awrite(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    void serve(const request * req) throw(io_error);
    ~mem_file();
    stxxl::int64 size();
    void set_size(stxxl::int64 newsize);
    void lock();
    void delete_region(int64 offset, unsigned_type size);
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_MEM_FILE_HEADER
