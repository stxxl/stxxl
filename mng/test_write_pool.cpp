/***************************************************************************
 *  mng/test_write_pool.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example mng/test_write_pool.cpp

#include <iostream>
#include <stxxl/mng>
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
    stxxl::write_pool<block_type> pool(100);
    pool.resize(10);
    pool.resize(5);
    block_type * blk = new block_type;
    block_type::bid_type bid;
    stxxl::block_manager::get_instance()->new_blocks(stxxl::single_disk(), &bid, (&bid) + 1);
    pool.write(blk, bid)->wait();
    delete blk;
}
