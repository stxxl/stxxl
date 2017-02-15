/***************************************************************************
 *  include/stxxl/bits/io/request.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_REQUEST_HEADER
#define STXXL_IO_REQUEST_HEADER

#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/common/counting_ptr.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/verbose.h>

#include <string>
#include <cassert>
#include <memory>
#include <functional>

namespace stxxl {

//! \addtogroup reqlayer
//! \{

#define STXXL_BLOCK_ALIGN 4096

class file;
class request;

//! A reference counting pointer for \c file.
using file_ptr = counting_ptr<file>;

//! A reference counting pointer for \c request.
using request_ptr = counting_ptr<request>;

using completion_handler = std::function<void(request* r, bool success)>;

//! Request object encapsulating basic properties like file and offset.
class request : virtual public request_interface, public reference_count
{
    friend class linuxaio_queue;

protected:
    completion_handler on_complete_;
    std::unique_ptr<io_error> error_;

protected:
    file* file_;
    void* buffer_;
    offset_type offset_;
    size_type bytes_;
    read_or_write op_;

public:
    request(const completion_handler& on_complete,
            file* file, void* buffer, offset_type offset, size_type bytes,
            read_or_write type);

    virtual ~request();

    file * get_file() const { return file_; }
    void * get_buffer() const { return buffer_; }
    offset_type get_offset() const { return offset_; }
    size_type get_size() const { return bytes_; }
    read_or_write get_op() const { return op_; }

    void check_alignment() const;

    std::ostream & print(std::ostream& out) const final;

    //! Inform the request object that an error occurred during the I/O
    //! execution.
    void error_occured(const char* msg)
    {
        error_.reset(new stxxl::io_error(msg));
    }

    //! Inform the request object that an error occurred during the I/O
    //! execution.
    void error_occured(const std::string& msg)
    {
        error_.reset(new stxxl::io_error(msg));
    }

    //! Rises an exception if there were error with the I/O.
    void check_errors()
    {
        if (error_.get())
            throw *(error_.get());
    }

    const char * io_type() const override;

protected:
    void check_nref(bool after = false)
    {
        if (get_reference_count() < 2)
            check_nref_failed(after);
    }

private:
    void check_nref_failed(bool after);
};

std::ostream& operator << (std::ostream& out, const request& req);

//! \}

} // namespace stxxl

#endif // !STXXL_IO_REQUEST_HEADER
// vim: et:ts=4:sw=4
