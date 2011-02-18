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

template <class IBT>
void set_pattern_A(IBT & ib)
{
    for (int_type i = 0; i < ib.size; ++i)
        ib[i] = i;
}

template <class IBT>
int_type test_pattern_A(IBT & ib)
{
    int_type num_err = 0;
    for (int_type i = 0; i < ib.size; ++i)
        num_err += (ib[i] != i);
    return num_err;
}

template <class IBT>
void set_pattern_B(IBT & ib)
{
    for (int_type i = 0; i < ib.size; ++i)
        ib[i] = ib.size - i;
}

template <class IBT>
int_type test_pattern_B(IBT & ib)
{
    int_type num_err = 0;
    for (int_type i = 0; i < ib.size; ++i)
        num_err += (ib[i] != ib.size - i);
    return num_err;
}

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

    typedef block_scheduler< swappable_block<value_type, block_size> > bst;
    typedef bst::swappable_block_identifier_type sbit;
    typedef bst::internal_block_type ibt;
    typedef bst::external_block_type ebt;

    switch (test_case)
    {
    case -1:
    case 0:
        {   // ------------------- call all functions -----------------------
            STXXL_MSG("next test: call all functions");
            ebt ext_bl; // prepare an external_block with pattern A
            block_manager::get_instance()->new_block(striping(), ext_bl);
            ibt * int_bl = new ibt;
            set_pattern_A(*int_bl);
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
            delete b_s;

            int_bl->read(ext_bl)->wait();
            int_type num_err = test_pattern_B(*int_bl); // check the block. it should contain pattern B.
            if (num_err)
                STXXL_ERRMSG("after extraction changed block had " << num_err << " errors.");
            delete int_bl;
        }
        {   // ---------- force swapping ---------------------
            STXXL_MSG("next test: force swapping");
            const int_type num_sb = 5;

            bst * b_s = new bst(block_size * sizeof(value_type) * 3); // only 3 internal_blocks allowed
            sbit sbi[num_sb];
            for (int_type i = 0; i < num_sb; ++i)
                sbi[i] = b_s->allocate_swappable_block();
            ibt * ib[num_sb];
            ib[0] = &b_s->acquire(sbi[0]); // fill 3 blocks
            ib[1] = &b_s->acquire(sbi[1]);
            ib[2] = &b_s->acquire(sbi[2]);
            set_pattern_A(*ib[0]);
            set_pattern_A(*ib[1]);
            set_pattern_A(*ib[2]);
            b_s->release(sbi[0], true);
            b_s->release(sbi[1], true);
            b_s->release(sbi[2], true);

            ib[3] = &b_s->acquire(sbi[3]); // fill 2 blocks, now some others have to be swapped out
            ib[4] = &b_s->acquire(sbi[4]);
            set_pattern_A(*ib[3]);
            set_pattern_A(*ib[4]);
            b_s->release(sbi[3], true);
            b_s->release(sbi[4], true);

            ib[2] = &b_s->acquire(sbi[2]); // this block can still be cached
            ib[3] = &b_s->acquire(sbi[3]); // as can this
            ib[1] = &b_s->acquire(sbi[1]); // but not this
            if (test_pattern_A(*ib[1]))
                STXXL_ERRMSG("Block 1 had errors.");
            if (test_pattern_A(*ib[2]))
                STXXL_ERRMSG("Block 2 had errors.");
            if (test_pattern_A(*ib[3]))
                STXXL_ERRMSG("Block 3 had errors.");
            b_s->release(sbi[1], false);
            b_s->release(sbi[2], false);
            b_s->release(sbi[3], false);
            for (int_type i = 0; i < num_sb; ++i)
                b_s->free_swappable_block(sbi[i]);
            delete b_s;
        }
        break;
    case 1:
        {   // ---------- do not free uninitialized block ---------------------
            STXXL_MSG("next test: do not free uninitialized block");

            bst * b_s = new bst(block_size * sizeof(value_type));
            sbit sbi;

            sbi = b_s->allocate_swappable_block();
            b_s->acquire(sbi);
            b_s->release(sbi, false);
            // do not free uninitialized block

            delete b_s;
        }
        break;
    case 2:
        {   // ---------- do not free initialized block ---------------------
            STXXL_MSG("next test: do not free initialized block");

            bst * b_s = new bst(block_size * sizeof(value_type));
            sbit sbi;

            sbi = b_s->allocate_swappable_block();
            b_s->acquire(sbi);
            b_s->release(sbi, true);
            // do not free initialized block

            delete b_s;
        }
        break;
    case 3:
        {   // ---------- do not release but free block ---------------------
            STXXL_MSG("next test: do not release but free block");

            bst * b_s = new bst(block_size * sizeof(value_type));
            sbit sbi;

            sbi = b_s->allocate_swappable_block();
            b_s->acquire(sbi);
            // do not release block
            b_s->free_swappable_block(sbi);

            delete b_s;
        }
        break;
    case 4:
        {   // ---------- do neither release nor free block ---------------------
            STXXL_MSG("next test: do neither release nor free block");

            bst * b_s = new bst(block_size * sizeof(value_type));
            sbit sbi;

            sbi = b_s->allocate_swappable_block();
            b_s->acquire(sbi);
            // do not release block
            // do not free block

            delete b_s;
        }
        break;
    case 5:
        {   // ---------- release block to often ---------------------
            STXXL_MSG("next test: release block to often");

            bst * b_s = new bst(block_size * sizeof(value_type));
            sbit sbi;

            sbi = b_s->allocate_swappable_block();
            b_s->acquire(sbi);
            b_s->release(sbi, false);
            b_s->release(sbi, false); // release once to often

            delete b_s;
        }
        break;
    }
    STXXL_MSG("end of test");
    return 0;
}







