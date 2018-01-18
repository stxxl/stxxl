/***************************************************************************
 *  examples/containers/stack1.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Daniel Feist <daniel.feist@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! [example]
#include <cassert>
#include <stxxl/stack>

int main()
{
    using stack_type = stxxl::STACK_GENERATOR<int>::result;
    stack_type my_stack;

    my_stack.push(8);
    my_stack.push(7);
    my_stack.push(4);
    assert(my_stack.size() == 3);

    assert(my_stack.top() == 4);
    my_stack.pop();

    assert(my_stack.top() == 7);
    my_stack.pop();

    assert(my_stack.top() == 8);
    my_stack.pop();

    assert(my_stack.empty());

    return 0;
}
//! [example]
