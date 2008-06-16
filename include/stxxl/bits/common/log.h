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

#include "stxxl/bits/namespace.h"

#include <iostream>
#include <fstream>


__STXXL_BEGIN_NAMESPACE

class logger
{
    static logger * instance;
    std::ofstream log_stream_;
    std::ofstream errlog_stream_;
    inline logger() :
        log_stream_("stxxl.log"),
        errlog_stream_("stxxl.errlog")
    { }

public:
    inline std::ofstream & log_stream()
    {
        return log_stream_;
    }

    inline std::ofstream & errlog_stream()
    {
        return errlog_stream_;
    }

    inline static logger * get_instance()
    {
        if (!instance)
            instance = new logger();

        return instance;
    }
};

__STXXL_END_NAMESPACE

#endif // !STXXL_LOG_HEADER
