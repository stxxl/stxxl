/***************************************************************************
 *  tests/mng/test_block_manager1.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/mng>
#include <stxxl/request>

int main()
{
    typedef stxxl::typed_block<128* 1024, double> block_type;
    std::vector<block_type::bid_type> bids(32);
    std::vector<stxxl::request_ptr> requests;
    stxxl::block_manager* bm = stxxl::block_manager::get_instance();
    bm->new_blocks(stxxl::striping(), bids.begin(), bids.end());
    std::vector<block_type, stxxl::new_alloc<block_type> > blocks(32);
    for (int vIndex = 0; vIndex < 32; ++vIndex) {
        for (size_t vIndex2 = 0; vIndex2 < block_type::size; ++vIndex2) {
            blocks[vIndex][vIndex2] = static_cast<double>(vIndex2);
        }
    }
    for (int vIndex = 0; vIndex < 32; ++vIndex) {
        requests.push_back(blocks[vIndex].write(bids[vIndex]));
    }
    stxxl::wait_all(requests.begin(), requests.end());
    bm->delete_blocks(bids.begin(), bids.end());
    return 0;
}
