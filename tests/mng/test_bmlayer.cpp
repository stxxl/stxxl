/***************************************************************************
 *  tests/mng/test_bmlayer.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <stxxl.h>

#define BLOCK_SIZE (1024 * 512)

struct MyType
{
    size_t integer;
    char chars[5];
};

struct my_handler
{
    void operator () (stxxl::request* req, bool /* success */)
    {
        STXXL_MSG(req << " done, type=" << req->io_type());
    }
};

using block_type = stxxl::typed_block<BLOCK_SIZE, MyType>;

void testIO()
{
    const unsigned nblocks = 2;
    stxxl::BIDArray<BLOCK_SIZE> bids(nblocks);
    std::vector<int> disks(nblocks, 2);
    stxxl::request_ptr* reqs = new stxxl::request_ptr[nblocks];
    stxxl::block_manager* bm = stxxl::block_manager::get_instance();
    bm->new_blocks(stxxl::striping(), bids.begin(), bids.end());

    block_type* block = new block_type;
    STXXL_MSG(std::hex);
    STXXL_MSG("Allocated block address    : " << (size_t)(block));
    STXXL_MSG("Allocated block address + 1: " << (size_t)(block + 1));
    STXXL_MSG(std::dec);
    size_t i = 0;
    for (i = 0; i < block_type::size; ++i)
    {
        block->elem[i].integer = i;
        //memcpy (block->elem[i].chars, "STXXL", 4);
    }
    for (i = 0; i < nblocks; ++i)
        reqs[i] = block->write(bids[i], my_handler());

    std::cout << "Waiting " << std::endl;
    stxxl::wait_all(reqs, nblocks);

    for (i = 0; i < nblocks; ++i)
    {
        reqs[i] = block->read(bids[i], my_handler());
        reqs[i]->wait();
        for (size_t j = 0; j < block_type::size; ++j)
        {
            STXXL_CHECK(j == block->elem[j].integer);
        }
    }

    bm->delete_blocks(bids.begin(), bids.end());

    delete[] reqs;
    delete block;
}

void testIO2()
{
    using block_type = stxxl::typed_block<128* 1024, double>;
    std::vector<block_type::bid_type> bids(32);
    std::vector<stxxl::request_ptr> requests;
    stxxl::block_manager* bm = stxxl::block_manager::get_instance();
    bm->new_blocks(stxxl::striping(), bids.begin(), bids.end());
    block_type* blocks = new block_type[32];
    int vIndex;
    for (vIndex = 0; vIndex < 32; ++vIndex) {
        for (size_t vIndex2 = 0; vIndex2 < block_type::size; ++vIndex2) {
            blocks[vIndex][vIndex2] = double(vIndex2);
        }
    }
    for (vIndex = 0; vIndex < 32; ++vIndex) {
        requests.push_back(blocks[vIndex].write(bids[vIndex]));
    }
    stxxl::wait_all(requests.begin(), requests.end());
    bm->delete_blocks(bids.begin(), bids.end());
    delete[] blocks;
}

void testPrefetchPool()
{
    stxxl::prefetch_pool<block_type> pool(2);
    pool.resize(10);
    pool.resize(5);
    block_type* blk = new block_type;
    block_type::bid_type bid;
    stxxl::block_manager::get_instance()->new_block(stxxl::single_disk(), bid);
    pool.hint(bid);
    pool.read(blk, bid)->wait();
    delete blk;
}

void testWritePool()
{
    stxxl::write_pool<block_type> pool(100);
    pool.resize(10);
    pool.resize(5);
    block_type* blk = new block_type;
    block_type::bid_type bid;
    stxxl::block_manager::get_instance()->new_block(stxxl::single_disk(), bid);
    pool.write(blk, bid);
}

using block_type1 = stxxl::typed_block<BLOCK_SIZE, int>;
using buf_ostream_type = stxxl::buf_ostream<block_type1, stxxl::BIDArray<BLOCK_SIZE>::iterator>;
using buf_istream_type = stxxl::buf_istream<block_type1, stxxl::BIDArray<BLOCK_SIZE>::iterator>;

void testStreams()
{
    const unsigned nblocks = 64;
    const unsigned nelements = nblocks * block_type1::size;
    stxxl::BIDArray<BLOCK_SIZE> bids(nblocks);

    stxxl::block_manager* bm = stxxl::block_manager::get_instance();
    bm->new_blocks(stxxl::striping(), bids.begin(), bids.end());
    {
        buf_ostream_type out(bids.begin(), 2);
        for (unsigned i = 0; i < nelements; i++)
            out << i;
    }
    {
        buf_istream_type in(bids.begin(), bids.end(), 2);
        for (unsigned i = 0; i < nelements; i++)
        {
            int value;
            in >> value;
            STXXL_CHECK(value == int(i));
        }
    }
    bm->delete_blocks(bids.begin(), bids.end());
}

int main()
{
    testIO();
    testIO2();
    testPrefetchPool();
    testWritePool();
    testStreams();

    stxxl::block_manager* bm = stxxl::block_manager::get_instance();
    STXXL_MSG("block_manager total allocation: " << bm->total_allocation());
    STXXL_MSG("block_manager current allocation: " << bm->current_allocation());
    STXXL_MSG("block_manager maximum allocation: " << bm->maximum_allocation());
}
