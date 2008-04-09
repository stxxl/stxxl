#include <iostream>

#include "stxxl.h"
#include "stxxl/bits/common/seed.h"
#include "stxxl/bits/containers/hash_map/block_cache.h"

bool test_block_cache() {
	try {
		const int subblock_size = 4;	// size in values
		const int block_size = 2;		// size in subblocks
		const int num_blocks = 64;
		
		const int cache_size = 8;		// size of cache in blocks
		
		typedef std::pair<int, int> value_type;
		
		const int subblock_raw_size = subblock_size*sizeof(value_type)+2;
		
		typedef stxxl::typed_block<subblock_raw_size, value_type> subblock_type;
		typedef stxxl::typed_block<block_size*sizeof(subblock_type), subblock_type> block_type;
		typedef block_type::bid_type bid_type;
		typedef std::vector<bid_type> bid_container_type;

		// prepare test: allocate blocks, fill them with values and write to disk
		bid_container_type bids(num_blocks);
		stxxl::block_manager *bm = stxxl::block_manager::get_instance ();
        bm->new_blocks (stxxl::striping(), bids.begin (), bids.end ());
		
		block_type *block = new block_type;
		for (unsigned i_block = 0; i_block < num_blocks; i_block++) {
			for (unsigned i_subblock = 0; i_subblock < block_size; i_subblock++) {
				for (unsigned i_value = 0; i_value < subblock_size; i_value++) {
					int value = i_value + i_subblock*subblock_size + i_block*block_size;
					(*block)[i_subblock][i_value] = value_type(value, value);
				}
			}
			stxxl::request_ptr req = block->write(bids[i_block]);
			req->wait();
		}
		
		stxxl::random_number32 rand32;
		
		// create block_cache
		typedef stxxl::hash_map::block_cache<block_type> cache_type;
		cache_type cache(cache_size);

		// load random subblocks and check for values
		int n_runs = cache_size*10;
		for (int i_run = 0; i_run<n_runs; i_run++) {
			int i_block = rand32() % num_blocks;
			int i_subblock = rand32() % block_size;
			
			subblock_type * subblock = cache.get_subblock(bids[i_block], i_subblock);
			
			int expected = i_block*block_size + i_subblock*subblock_size + 1;
			assert((*subblock)[1].first == expected);
		}
		
		// do the same again but this time with prefetching
		for (int i_run = 0; i_run<n_runs; i_run++) {
			int i_block = rand32() % num_blocks;
			int i_subblock = rand32() % block_size;
			
			cache.prefetch_block(bids[i_block]);
			subblock_type * subblock = cache.get_subblock(bids[i_block], i_subblock);
//			std::cout << (*subblock)[1].first << std::endl;
			int expected = i_block*block_size + i_subblock*subblock_size + 1;
			assert((*subblock)[1].first == expected);
		}
		
		// load and modify some subblocks; flush cache and check values
		unsigned myseed = stxxl::get_next_seed();
		stxxl::set_seed(myseed);
		for (int i_run = 0; i_run<n_runs; i_run++) {
			int i_block = rand32() % num_blocks;
			int i_subblock = rand32() % block_size;
			
			subblock_type * subblock = cache.get_subblock(bids[i_block], i_subblock);

			assert(cache.make_dirty(bids[i_block]));
			(*subblock)[1].first = (*subblock)[1].second + 42;
		}
		stxxl::set_seed(myseed);
		for (int i_run = 0; i_run<n_runs; i_run++) {
			int i_block = rand32() % num_blocks;
			int i_subblock = rand32() % block_size;
			subblock_type * subblock = cache.get_subblock(bids[i_block], i_subblock);

			int expected = i_block*block_size + i_subblock*subblock_size + 1;
			assert((*subblock)[1].first == expected+42);
		}
		
		// test retaining
		cache.clear();
		assert(cache.retain_block(bids[0]) == false); // not yet cached
		cache.prefetch_block(bids[0]);
		assert(cache.retain_block(bids[0]) == true);
		assert(cache.release_block(bids[0]) == true);
		assert(cache.release_block(bids[0]) == false);
		
		subblock_type * retained_subblock = cache.get_subblock(bids[1], 0);
		assert(cache.retain_block(bids[1]) == true);
		for (int i = 0; i < cache_size+5; i++) {
			cache.prefetch_block(bids[i+3]);
		}
		assert(cache.get_subblock(bids[1], 0) == retained_subblock);	// retained_subblock should not have been kicked 
		cache.clear();
		
		// test swapping
		subblock_type * a_subblock = cache.get_subblock(bids[6], 1);
		cache_type cache2(cache_size/2);
		std::swap(cache, cache2);
		assert(cache2.get_subblock(bids[6], 1) == a_subblock);
		
		
		cache.__dump_cache();
		
	}
	catch(...) {
		STXXL_MSG("Cought unknown exception.")
		return false;		
	}
	return true;
}


int main() {
	test_block_cache();
	return 0;
}

