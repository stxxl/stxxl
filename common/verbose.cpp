/***************************************************************************
 *  common/verbose.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <cstdio>
#include <cmath>
#include <stxxl/bits/verbose.h>
#include <stxxl/bits/common/log.h>
#include <stxxl/bits/common/timer.h>


__STXXL_BEGIN_NAMESPACE

static const double program_start_time_stamp = timestamp();

void print_msg(const char * label, const std::string & msg, unsigned flags)
{
    std::string s;
    if (flags & _STXXL_PRNT_TIMESTAMP) {
        double t = timestamp() - program_start_time_stamp;
        char tstr[23]; /* "[364:23:59:59.999999] " */
        snprintf(tstr, sizeof(tstr), "[%d.%02d:%02d:%02d.%06d] ",
                int(t / (24 * 60 * 60)),
                int(t / (60 * 60)) % 24,
                int(t / 60) % 60, int(t) % 60,
                int((t - floor(t)) * 10000));
        s += tstr;
    }
    if (label) {
        s += '[';
        s += label;
        s += "] ";
    }
    s += msg;
    if (flags & _STXXL_PRNT_ADDNEWLINE)
        s += '\n';
    if (flags & _STXXL_PRNT_COUT)
        std::cout << s << std::flush;
    if (flags & _STXXL_PRNT_CERR)
        std::cerr << s << std::flush;
    logger * logger_instance = logger::get_instance();
    if (flags & _STXXL_PRNT_LOG)
        logger_instance->log_stream() << s << std::flush;
    if (flags & _STXXL_PRNT_ERRLOG)
        logger_instance->errlog_stream() << s << std::flush;
}

__STXXL_END_NAMESPACE

// vim: et:ts=4:sw=4
