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
    const int block_size = 1024;
    typedef int_type value_type;

    int test_case = -1;
    int_type internal_memory = 256 * 1024 * 1024;

    dsr::Argument_helper ah;
    ah.new_named_int('t', "test-case", "I", "number of the test case to run", test_case);
    ah.new_named_int('m', "memory", "N", "internal memory to use (in bytes)", internal_memory);
    ah.set_description("stxxl block_scheduler test");
    ah.set_author("Raoul Steffen, R-Steffen@gmx.de");
    ah.process(argc, argv);


    // ------------------- call all functions -----------------------
    typedef block_scheduler< swappable_block<value_type, block_size> > bst;
    typedef bst::swappable_block_identifier_type sbit;
    typedef bst::internal_block_type ibt;
    typedef bst::external_block_type ebt;

    ebt ext_bl; // prepare an external_block with pattern A
    block_manager::get_instance()->new_block(striping(), ext_bl);
    ibt * int_bl = new ibt;
    for (int_type i = 0; i < block_size; ++i)
        int_bl->elem[i] = i;
    int_bl->write(ext_bl)->wait();

    bst * b_s = new bst(internal_memory); // the block_scheduler may use internal_memory byte for caching
    bst & bs = *b_s;
    assert(! bs.is_simulating()); // make sure is not just recording a prediction sequence

    sbit sbi1 = bs.allocate_swappable_block(); // allocate a swappable_block and store its identifier
    if (bs.is_initialized(sbi1)) // it should not be initialized
            STXXL_ERRMSG("new block is initialized");
    bs.initialize(sbi1, ext_bl); // initialize the swappable_block with the prepared external_block
    {
        ibt & ib = bs.acquire(sbi1); // acquire the swappable_block to gain access
        int_type num_err = 0;
        for (int_type i = 0; i < block_size; ++i)
            num_err += (ib[i] != i); // read from the swappable_block. it should contain pattern A
        if (num_err)
            STXXL_ERRMSG("previously initialized block had " << num_err << " errors.");
    }
    {
        ibt & ib = bs.get_internal_block(sbi1); // get a new reference to the already allocated block (because we forgot the old one)
        for (int_type i = 0; i < block_size; ++i)
            ib[i] = block_size - i; // write pattern B
        bs.release(sbi1, true); // release the swappable_block. changes have to be stored. it may now be swapped out.
    }
    sbit sbi2 = bs.allocate_swappable_block(); // allocate a second swappable_block and store its identifier
    if (bs.is_initialized(sbi2)) // it should not be initialized
        STXXL_ERRMSG("new block is initialized");
    {
        ibt & ib1 = bs.acquire(sbi1); // acquire the swappable_block to gain access
        ibt & ib2 = bs.acquire(sbi2); // acquire the swappable_block to gain access because it was uninitialized, it now becomes initialized
        for (int_type i = 0; i < block_size; ++i)
            ib2[i] = ib1[i]; // copy pattern B
        bs.release(sbi1, false); // release the swappable_block. no changes happened.
        bs.release(sbi2, true);
    }
    if (! bs.is_initialized(sbi1)) // both blocks should now be initialized
            STXXL_ERRMSG("block is not initialized");
    if (! bs.is_initialized(sbi2)) // both blocks should now be initialized
                STXXL_ERRMSG("block is not initialized");
    ext_bl = bs.extract_external_block(sbi2); // get the external_block
    if (bs.is_initialized(sbi2)) // should not be initialized any more
        STXXL_ERRMSG("block is initialized after extraction");
    bs.deinitialize(sbi1);
    if (bs.is_initialized(sbi1)) // should not be initialized any more
        STXXL_ERRMSG("block is initialized after deinitialize");
    bs.free_swappable_block(sbi1); // free the swappable_blocks
    bs.free_swappable_block(sbi2);
    bs.explicit_timestep();

    int_bl->read(ext_bl)->wait();
    int_type num_err = 0;
    for (int_type i = 0; i < block_size; ++i)
        num_err += (int_bl->elem[i] != block_size - i); // check the block. it should contain pattern B.
    if (num_err)
        STXXL_ERRMSG("after extraction changed block had " << num_err << " errors.");

    delete int_bl;
    return 0;
}
