/***************************************************************************
 *  tests/containers/test_queue2.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/queue>

using my_type = uint64_t;

// forced instantiation
template class stxxl::queue<my_type>;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " [n in MiB]" << std::endl;
        return -1;
    }

    size_t mega = 1 << 20;
    size_t megabytes = atoi(argv[1]);
    size_t nelements = megabytes * mega / sizeof(my_type);
    size_t i;
    my_type in = 0, out = 0;

    stxxl::queue<my_type> q;

    LOG1 << "op-sequence: ( push, pop, push ) * n";
    for (i = 0; i < nelements; ++i)
    {
        if ((i % mega) == 0)
            LOG1 << "Insert " << i;

        q.push(in++);

        die_unless(q.front() == out);
        q.pop();
        ++out;

        q.push(in++);
    }
    LOG1 << "op-sequence: ( pop, push, pop ) * n";
    for ( ; i > 0; --i)
    {
        if ((i % mega) == 0)
            LOG1 << "Remove " << i;

        die_unless(q.front() == out);
        q.pop();
        ++out;

        q.push(in++);

        die_unless(q.front() == out);
        q.pop();
        ++out;
    }
    die_unless(q.empty());
    die_unless(in == out);

    std::cout << *foxxll::stats::get_instance();

    return 0;
}
