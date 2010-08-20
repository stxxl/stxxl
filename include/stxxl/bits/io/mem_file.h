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

#include <stxxl/bits/io/disk_queued_file.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on new[] and memcpy
class mem_file : public disk_queued_file
{
    char * ptr;
    offset_type sz;

public:
    //! \brief constructs file object
    //! \param disk disk(file) identifier
    mem_file(
        int queue_id = DEFAULT_QUEUE, int allocator_id = NO_ALLOCATOR) : disk_queued_file(queue_id, allocator_id), ptr(NULL), sz(0)
    { }
    void serve(const request * req) throw (io_error);
    ~mem_file();
    offset_type size();
    void set_size(offset_type newsize);
    void lock();
    void discard(offset_type offset, offset_type size);
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_MEM_FILE_HEADER
