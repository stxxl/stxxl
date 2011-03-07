/***************************************************************************
 *  include/stxxl/bits/io/boostfd_file.h
 *
 *  File implementation based on boost::iostreams::file_decriptor
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_BOOSTFD_FILE_H_
#define STXXL_BOOSTFD_FILE_H_

#ifndef STXXL_HAVE_BOOSTFD_FILE
#ifdef STXXL_BOOST_CONFIG // if boost is available
 #define STXXL_HAVE_BOOSTFD_FILE 1
#else
 #define STXXL_HAVE_BOOSTFD_FILE 0
#endif
#endif

#if STXXL_HAVE_BOOSTFD_FILE

#include <stxxl/bits/io/disk_queued_file.h>
#include <stxxl/bits/io/request.h>

#include <boost/iostreams/device/file_descriptor.hpp>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation based on boost::iostreams::file_decriptor
class boostfd_file : public disk_queued_file
{
    typedef boost::iostreams::file_descriptor fd_type;

protected:
    mutex fd_mutex;        // sequentialize function calls involving file_des
    fd_type file_des;
    int mode_;
    offset_type _size();

public:
    boostfd_file(const std::string & filename, int mode, int queue_id = DEFAULT_QUEUE, int allocator_id = NO_ALLOCATOR);
    ~boostfd_file();
    offset_type size();
    void set_size(offset_type newsize);
    void lock();
    void serve(const request * req) throw (io_error);
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif  // #if STXXL_HAVE_BOOSTFD_FILE

#endif  // !STXXL_BOOSTFD_FILE_H_
