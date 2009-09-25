/***************************************************************************
 *  include/stxxl/bits/common/log.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2004-2005 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_LOG_HEADER
#define STXXL_LOG_HEADER

#include <iostream>
#include <fstream>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/singleton.h>


__STXXL_BEGIN_NAMESPACE

class logger : public singleton<logger>
{
    friend class singleton<logger>;

    std::ofstream log_stream_;
    std::ofstream errlog_stream_;
    std::ofstream * waitlog_stream_;

    inline logger() : waitlog_stream_(NULL)
    {
        const char * log_filename = getenv("STXXLLOGFILE");
        log_stream_.open(log_filename == NULL ? "stxxl.log" : log_filename);
        const char * errlog_filename = getenv("STXXLERRLOGFILE");
        errlog_stream_.open(errlog_filename == NULL ? "stxxl.errlog" : errlog_filename);
#ifdef STXXL_WAIT_LOG_ENABLED
        const char * waitlog_filename = getenv("STXXLWAITLOGFILE");
        waitlog_stream_ = new std::ofstream();
        waitlog_stream_->open(waitlog_filename == NULL ? "stxxl.waitlog" : waitlog_filename);
        *waitlog_stream_ << "# time\trd_incr\twr_incr\tw_read\tw_write" << std::endl;
#endif
    }
    
    ~logger()
    {
        delete waitlog_stream_;
    }

public:
    inline std::ofstream & log_stream()
    {
        return log_stream_;
    }

    inline std::ofstream & errlog_stream()
    {
        return errlog_stream_;
    }

    inline std::ofstream & waitlog_stream()
    {
        return *waitlog_stream_;
    }
};

__STXXL_END_NAMESPACE

#endif // !STXXL_LOG_HEADER
