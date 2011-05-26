/***************************************************************************
 *  include/stxxl/bits/version.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_VERSION_HEADER
#define STXXL_VERSION_HEADER

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

#define STXXL_VERSION_MAJOR             1
#define STXXL_VERSION_MINOR             3
#define STXXL_VERSION_PATCHLEVEL        2

const char * get_version_string();

int version_major();
int version_minor();
int version_patchlevel();

inline int check_library_version()
{
    if (version_major() != STXXL_VERSION_MAJOR)
        return 1;
    if (version_minor() != STXXL_VERSION_MINOR)
        return 2;
    if (version_patchlevel() != STXXL_VERSION_PATCHLEVEL)
        return 3;
    return 0;
}

__STXXL_END_NAMESPACE

#endif // !STXXL_VERSION_HEADER
