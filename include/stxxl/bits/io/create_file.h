/***************************************************************************
 *  include/stxxl/bits/io/create_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_CREATE_FILE_HEADER
#define STXXL_IO_CREATE_FILE_HEADER

#include <stxxl/bits/io/file.h>


__STXXL_BEGIN_NAMESPACE

file * create_file(const std::string & io_impl,
                   const std::string & filename,
                   int options,
                   int physical_device_id = file::DEFAULT_QUEUE,
                   int allocator_id = file::NO_ALLOCATOR);

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_CREATE_FILE_HEADER
// vim: et:ts=4:sw=4
