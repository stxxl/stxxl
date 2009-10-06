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
        int disk = -1) : disk_queued_file(disk), ptr(NULL), sz(0)
    { }
    void serve(const request * req) throw (io_error);
    ~mem_file();
    offset_type size();
    void set_size(offset_type newsize);
    void lock();
    void delete_region(offset_type offset, size_type size);
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_MEM_FILE_HEADER
