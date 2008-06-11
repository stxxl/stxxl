/***************************************************************************
 *            test_prefetch_pool.cpp
 *
 *  Wed Jul  2 15:32:34 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include <iostream>
#include "stxxl/bits/mng/mng.h"
#include "stxxl/bits/mng/prefetch_pool.h"

//! \example mng/test_prefetch_pool.cpp

#define BLOCK_SIZE (1024 * 512)

struct MyType
{
    int integer;
    char chars[5];
};

typedef stxxl::typed_block<BLOCK_SIZE, MyType> block_type;

int main ()
{
    stxxl::prefetch_pool<block_type> pool(2);
    pool.resize(10);
    pool.resize(5);
    block_type * blk = new block_type;
    block_type::bid_type bid;
    stxxl::block_manager::get_instance()->new_blocks(stxxl::single_disk(), &bid, (&bid) + 1);
    pool.hint(bid);
    pool.read(blk, bid)->wait();
    delete blk;
}
