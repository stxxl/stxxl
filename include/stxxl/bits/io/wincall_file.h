/***************************************************************************
 *  include/stxxl/bits/io/wincall_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005-2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_WINCALL_FILE_HEADER
#define STXXL_WINCALL_FILE_HEADER

#ifndef STXXL_HAVE_WINCALL_FILE
#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC
 #define STXXL_HAVE_WINCALL_FILE 1
#else
 #define STXXL_HAVE_WINCALL_FILE 0
#endif
#endif

#if STXXL_HAVE_WINCALL_FILE

#include <stxxl/bits/io/wfs_file_base.h>
#include <stxxl/bits/io/disk_queued_file.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on Windows native I/O calls
class wincall_file : public wfs_file_base, public disk_queued_file
{
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    wincall_file(
        const std::string & filename,
        int mode,
        int queue_id = DEFAULT_QUEUE, int allocator_id = NO_ALLOCATOR)
        : wfs_file_base(filename, mode), disk_queued_file(queue_id, allocator_id)
    { }
    void serve(const request * req) throw (io_error);
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif  // #if STXXL_HAVE_WINCALL_FILE

#endif  // !STXXL_WINCALL_FILE_HEADER
