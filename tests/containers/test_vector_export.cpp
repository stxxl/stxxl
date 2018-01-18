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

#include <stxxl/scan>
#include <stxxl/vector>

int main()
{
    // use non-randomized striping to avoid side effects on random generator
    using vector_type = stxxl::vector<int64_t, 2, stxxl::lru_pager<2>, (2* 1024* 1024), foxxll::striping>;
    vector_type v(size_t(64 * 1024 * 1024) / sizeof(int64_t));

    stxxl::random_number32 rnd;
    int offset = rnd();

    LOG1 << "write " << v.size() << " elements";

    stxxl::ran32State = 0xdeadbeef;
    vector_type::size_type i;

    // fill the vector with increasing sequence of integer numbers
    for (i = 0; i < v.size(); ++i)
    {
        v[i] = i + offset;
        die_unless(v[i] == int64_t(i + offset));
    }

    v.flush();

    LOG1 << "export files";
    v.export_files("exported_");

    return 0;
}
