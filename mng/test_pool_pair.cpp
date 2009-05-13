/***************************************************************************
 *  mng/test_prefetch_pool.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example mng/test_prefetch_pool.cpp

#include <iostream>
#include <stxxl/mng>
#include <stxxl/bits/mng/prefetch_pool.h>
#include <stxxl/bits/mng/write_pool.h>

#define BLOCK_SIZE (1024 * 512)

struct MyType
{
    int integer;
    char chars[5];
};

typedef stxxl::typed_block<BLOCK_SIZE, MyType> block_type;

int main()
{
    stxxl::block_manager *bm = stxxl::block_manager::get_instance();
    STXXL_DEFAULT_ALLOC_STRATEGY alloc;

    {
        STXXL_MSG("Write-After-Hint coherence test #1");
        stxxl::prefetch_pool<block_type> p_pool(1);
        stxxl::write_pool<block_type> w_pool(1);
        block_type * blk;
        block_type::bid_type bids[2];

        bm->new_blocks(alloc, bids, bids + 2);
        blk = w_pool.steal();
        (*blk)[0].integer = 42;
        w_pool.write(blk, bids[0]);
        blk = w_pool.steal(); // flush w_pool

        // hint the block
        p_pool.hint(bids[0]);

        // update the hinted block
        (*blk)[0].integer = 23;
        w_pool.write(blk, bids[0]);
        blk = w_pool.steal(); // flush w_pool

        // get the hinted block
        p_pool.read(blk, bids[0])->wait();

        if ((*blk)[0].integer != 23) {
            STXXL_ERRMSG("WRITE-AFTER-HINT COHERENCE FAILURE");
        }

        w_pool.add(blk);
        bm->delete_blocks(&bids[0], &bids[2]);
    }

    {
        STXXL_MSG("Write-After-Hint coherence test #2");
        stxxl::prefetch_pool<block_type> p_pool(1);
        stxxl::write_pool<block_type> w_pool(1);
        block_type * blk;
        block_type::bid_type bids[2];

        bm->new_blocks(alloc, bids, bids + 2);
        blk = w_pool.steal();
        (*blk)[0].integer = 42;
        w_pool.write(blk, bids[0]);

        // hint the block
        p_pool.hint(bids[0], w_pool); // flush w_pool

        // update the hinted block
        blk = w_pool.steal();
        (*blk)[0].integer = 23;
        w_pool.write(blk, bids[0]);
        blk = w_pool.steal(); // flush w_pool

        // get the hinted block
        p_pool.read(blk, bids[0])->wait();

        if ((*blk)[0].integer != 23) {
            STXXL_ERRMSG("WRITE-AFTER-HINT COHERENCE FAILURE");
        }

        w_pool.add(blk);
        bm->delete_blocks(&bids[0], &bids[2]);
    }
}
// vim: et:ts=4:sw=4
