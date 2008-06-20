/***************************************************************************
 *  containers/test_many_stacks.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/test_many_stacks.cpp
//! This is an example of how to use \c stxxl::STACK_GENERATOR class
//! to generate an \b external stack type
//! with \c stxxl::grow_shrink_stack implementation, \b four blocks per page,
//! block size \b 4096 bytes


#include <stxxl/stack>


int main(int argc, char * argv[])
{
    typedef stxxl::STACK_GENERATOR<int, stxxl::external, stxxl::normal, 4, 4096>::result ext_stack_type;

    if (argc < 2)
    {
        STXXL_MSG("Usage: " << argv[0] << " number_of_stacks");
        abort();
    }

    char dum;
    STXXL_MSG("Enter a symbol:");
    std::cin >> dum;

    ext_stack_type * my_stacks = new ext_stack_type[atoi(argv[1])];

    STXXL_MSG("Enter a symbol:");
    std::cin >> dum;

    delete[] my_stacks;

    STXXL_MSG("Enter a symbol:");
    std::cin >> dum;

    return 0;
}
