/***************************************************************************
 *  lib/common/utils.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/common/utils.h>

#include <sstream>
#include <iomanip>

__STXXL_BEGIN_NAMESPACE

//! Parse a string like "343KB" or "44 GiB" into the corresponding size in
//! bytes. Returns the number of bytes and sets ok = true if the string could
//! be parsed correctly.
bool parse_SI_IEC_size(const std::string& str, uint64& size)
{
    char* endptr;
    size = strtoul(str.c_str(), &endptr, 10);
    if (!endptr) return false; // parse failed, no number

    while (endptr[0] == ' ') ++endptr; // skip over spaces

    if ( endptr[0] == 0 ) // number parsed, no suffix defaults to MiB
        size *= 1024 * 1024;
    else if ( (endptr[0] == 'b' || endptr[0] == 'B') && endptr[1] == 0 ) // bytes
        size *= 1;
    else if (endptr[0] == 'k' || endptr[0] == 'K')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= 1000; // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= 1024; // power of two
        else
            return false;
    }
    else if (endptr[0] == 'm' || endptr[0] == 'M')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= 1000 * 1000; // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= 1024 * 1024; // power of two
        else
            return false;
    }
    else if (endptr[0] == 'g' || endptr[0] == 'G')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= 1000 * 1000 * 1000; // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= 1024 * 1024 * 1024; // power of two
        else
            return false;
    }
    else if (endptr[0] == 't' || endptr[0] == 'T')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= int64(1000) * int64(1000) * int64(1000) * int64(1000); // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= int64(1024) * int64(1024) * int64(1024) * int64(1024); // power of two
        else
            return false;
    }
    else if (endptr[0] == 'p' || endptr[0] == 'P')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= int64(1000) * int64(1000) * int64(1000) * int64(1000) * int64(1000); // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= int64(1024) * int64(1024) * int64(1024) * int64(1024) * int64(1024); // power of two
        else
            return false;
    }
    else
        return false;

    return true;
}

std::string format_SI_size(uint64 number)
{
    // may not overflow, std::numeric_limits<uint64>::max() == 16 EiB
    double multiplier = 1000.0;
    static const char * SIendings[] = { "", "k", "M", "G", "T", "P", "E" };
    unsigned int scale = 0;
    double number_d = (double)number;
    while (number_d >= multiplier) {
        number_d /= multiplier;
        ++scale;
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << number_d
        << ' ' << SIendings[scale];
    return out.str();
}

std::string format_IEC_size(uint64 number)
{
    // may not overflow, std::numeric_limits<uint64>::max() == 16 EiB
    double multiplier = 1024.0;
    static const char * IECendings[] = { "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei" };
    unsigned int scale = 0;
    double number_d = (double)number;
    while (number_d >= multiplier) {
        number_d /= multiplier;
        ++scale;
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << number_d
        << ' ' << IECendings[scale];
    return out.str();
}

__STXXL_END_NAMESPACE
