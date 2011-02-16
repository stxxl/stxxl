/***************************************************************************
 *  containers/test_block_scheduler.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/mng/block_scheduler.h>
//#include <stxxl/vector>
//#include <stxxl/stream>

// Thanks Daniel Russel, Stanford University
#include <Argument_helper.h>

#include <iostream>
#include <limits>

using namespace stxxl;

int main(int argc, char **argv)
{
    typedef int_type value_type;

    int test_case = -1;
    int_type internal_memory = 256 * 1024 * 1024;

    dsr::Argument_helper ah;
    ah.new_int("test_case", "number of the test case to run", test_case);
    int internal_memory_megabyte;
    ah.new_named_int('m', "memory", "integer", "internal memory to use (in megabytes)", internal_memory_megabyte);

    ah.set_description("stxxl matrix test");
    ah.set_author("Raoul Steffen, R-Steffen@gmx.de");
    ah.process(argc, argv);

    internal_memory = int_type(internal_memory_megabyte) * 1048576;

    /*
    const int block_size = 1024;
    // call all functions
    typedef block_scheduler< swappable_block<value_type, block_size> > bs_t;
    bs_t bs(internal_memory);
    bs_t::external_block_type ext_bl;
    block_manager::get_instance()->new_block(striping(), ext_bl);

    bs_t::internal_block_type * int_bl = new bs_t::internal_block_type;
    for (int_type i = 1; i <= block_size; ++i)
        int_bl[i] = i;
    int_bl->write(ext_bl)->wait();
    delete int_bl;


    //*/
    return 0;
}
