/***************************************************************************
 *  tests/containers/test_vector_export.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2008 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/test_vector_export.cpp
//! This is an example of use of \c stxxl::vector::export_files

#include <algorithm>
#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/timer.hpp>

#include <stxxl/scan>
#include <stxxl/vector>

#include <test_helpers.h>

constexpr size_t export_size = 64 * 1024 * 1024;

int main()
{
    // use non-randomized striping to avoid side effects on random generator
    using vector_type = stxxl::vector<int64_t, 2, stxxl::lru_pager<2>, (2* 1024* 1024), foxxll::striping>;
    vector_type v(export_size / sizeof(int64_t));

    random_fill_vector(v);
    v.flush();

    LOG1 << "export files";
    v.export_files("exported_");

    return 0;
}
