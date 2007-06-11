/***************************************************************************
 *            test_stack.cpp
 *
 *  Thu May  1 22:02:00 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/stack"

//! \example containers/test_many_stacks.cpp
//! This is an example of how to use \c stxxl::STACK_GENERATOR class
//! to generate an \b external stack type
//! with \c stxxl::grow_shrink_stack implementation, \b four blocks per page,
//! block size \b 4096 bytes

using namespace stxxl;

int main(int argc, char * argv[])
{
    typedef STACK_GENERATOR < int, external, normal, 4, 4096 > ::result ext_stack_type;

    if (argc < 2)
    {
        STXXL_MSG("Usage: " << argv[0] << " number_of_stacks")
        abort();
    }

    char dum;
    STXXL_MSG("Enter a symbol:")
    std::cin >> dum;

    ext_stack_type * my_stacks = new ext_stack_type[atoi(argv[1])];

    STXXL_MSG("Enter a symbol:")
    std::cin >> dum;

    delete [] my_stacks;

    STXXL_MSG("Enter a symbol:")
    std::cin >> dum;

    return 0;
}
