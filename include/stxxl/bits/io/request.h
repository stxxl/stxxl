/***************************************************************************
 *  include/stxxl/bits/io/request.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO__REQUEST_H_
#define STXXL_IO__REQUEST_H_

#include <cassert>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/io/completion_handler.h>
#include <stxxl/bits/compat_unique_ptr.h>
#include <stxxl/bits/verbose.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

#define BLOCK_ALIGN 4096

class file;
class request_ptr;

//! \brief Request with basic properties like file and offset.
class request : virtual public request_interface
{
    friend class request_ptr;

protected:
    completion_handler on_complete;
    int ref_cnt;
    compat_unique_ptr<stxxl::io_error>::result error;

    mutex ref_cnt_mutex;

protected:
    file * file_;
    void * buffer;
    offset_type offset;
    size_type bytes;
    request_type type;

    void completed();

public:
    // returns number of references
    int nref()
    {
        scoped_mutex_lock Lock(ref_cnt_mutex);
        return ref_cnt;
    }

    request(const completion_handler & on_compl,
            file * file__,
            void * buffer_,
            offset_type offset_,
            size_type bytes_,
            request_type type_);

    virtual ~request();

    file * get_file() const { return file_; }
    void * get_buffer() const { return buffer; }
    offset_type get_offset() const { return offset; }
    size_type get_size() const { return bytes; }
    request_type get_type() const { return type; }

    void check_alignment() const;

    std::ostream & print(std::ostream & out) const;

    //! \brief Inform the request object that an error occurred
    //! during the I/O execution
    void error_occured(const char * msg)
    {
        error.reset(new stxxl::io_error(msg));
    }

    //! \brief Inform the request object that an error occurred
    //! during the I/O execution
    void error_occured(const std::string & msg)
    {
        error.reset(new stxxl::io_error(msg));
    }

    //! \brief Rises an exception if there were error with the I/O
    void check_errors() throw (stxxl::io_error)
    {
        if (error.get())
            throw *(error.get());
    }

private:
    void add_ref()
    {
        scoped_mutex_lock Lock(ref_cnt_mutex);
        ref_cnt++;
        STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request::add_ref(): added reference, ref_cnt=" << ref_cnt);
    }

    bool sub_ref()
    {
        int val;
        {
            scoped_mutex_lock Lock(ref_cnt_mutex);
            val = --ref_cnt;
            STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request::sub_ref(): subtracted reference, ref_cnt=" << ref_cnt);
        }
        assert(val >= 0);
        return (val == 0);
    }

protected:
    void check_nref(bool after = false)
    {
        if (nref() < 2)
            check_nref_failed(after);
    }

private:
    void check_nref_failed(bool after);
};

inline std::ostream & operator << (std::ostream & out, const request & req)
{
    return req.print(out);
}

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO__REQUEST_H_
// vim: et:ts=4:sw=4
