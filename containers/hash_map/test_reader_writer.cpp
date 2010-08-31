#include <iostream>

#include "stxxl.h"
#include "stxxl/bits/common/seed.h"
#include "stxxl/bits/containers/hash_map/util.h"

void reader_writer_test()
{
    try {
/*		const int subblock_size = 8;	// size in values
                const int block_size = 4;		// size in subblocks

                typedef std::pair<int, int> value_type;

                const int subblock_raw_size = subblock_size*sizeof(value_type);
*/

        typedef std::pair<unsigned, unsigned> value_type;

        const unsigned subblock_raw_size = 1024 * 8;                         // 8KB subblocks
        const unsigned block_size = 128;                                     // 1MB blocks (=128 subblocks)

        const unsigned n_blocks = 64;                                        // number of blocks to use for this test
        const unsigned cache_size = 8;                                       // size of cache in blocks

        const unsigned buffer_size = 4;                                      // write buffer size in blocks


        typedef stxxl::typed_block<subblock_raw_size, value_type> subblock_type;
        typedef stxxl::typed_block<block_size * sizeof(subblock_type), subblock_type> block_type;

        const unsigned subblock_size = subblock_type::size;  // size in values

        typedef block_type::bid_type bid_type;
        typedef std::vector<bid_type> bid_container_type;
        typedef bid_container_type::iterator bid_iterator_type;

        typedef stxxl::hash_map::block_cache<block_type> cache_type;

        typedef stxxl::hash_map::buffered_writer<block_type, bid_container_type> writer_type;
        typedef stxxl::hash_map::buffered_reader<cache_type, bid_iterator_type> reader_type;


        bid_container_type bids;
        cache_type cache(cache_size);

        // plain writing
        {
            writer_type writer(&bids, buffer_size, buffer_size / 2);
            unsigned i = 0;
            for (; i < n_blocks * block_size * subblock_size; ++i)
                writer.append(value_type(i, i));
            writer.flush();

            assert(bids.size() >= n_blocks);

            block_type * block = new block_type;
            i = 0;
            for (unsigned i_block = 0; i_block < n_blocks; i_block++) {
                stxxl::request_ptr req = block->read(bids[i_block]);
                req->wait();

                for (unsigned inner = 0; inner < block_size * subblock_size; ++inner) {
                    assert((*block)[inner / subblock_size][inner % subblock_size].first == i);
                    i++;
                }
            }
            delete block;
        }

        // reading with/without prefetching
        {
            reader_type reader(bids.begin(), bids.end(), &cache, 0, false);             // last parameter disables prefetching
            for (unsigned i = 0; i < n_blocks * block_size * subblock_size; ++i) {
                assert(reader.const_value().first == i);
                ++reader;
            }

            reader_type reader2(bids.begin(), bids.end(), &cache);              // prefetching enabled by default
            for (unsigned i = 0; i < n_blocks * block_size * subblock_size; ++i) {
                assert(reader2.const_value().first == i);
                ++reader2;
            }
        }

        // reading with skipping
        {
            reader_type reader(bids.begin(), bids.end(), &cache, 0, false);              // disable prefetching

            // I: first subblock
            reader.skip_to(bids.begin() + 10, 0);
            unsigned expected = block_size * subblock_size * 10 + subblock_size * 0;
            assert(reader.const_value().first == expected);

            // II: subblock in the middle (same block)
            reader.skip_to(bids.begin() + 10, 2);
            expected = block_size * subblock_size * 10 + subblock_size * 2;
            assert(reader.const_value().first == expected);

            // III: subblock in the middle (another block)
            reader.skip_to(bids.begin() + 13, 1);
            expected = block_size * subblock_size * 13 + subblock_size * 1;
            assert(reader.const_value().first == expected);
        }
//		cache.clear();


        // reading with modifying access
        {
            reader_type reader(bids.begin(), bids.end(), &cache);
            for (unsigned i = 0; i < n_blocks * block_size * subblock_size; ++i) {
                reader.value().second = reader.const_value().first + 1;
                ++reader;
            }

            reader_type reader2(bids.begin(), bids.end(), &cache);
            for (unsigned i = 0; i < n_blocks * block_size * subblock_size; ++i) {
                assert(reader2.const_value().second == reader2.const_value().first + 1);
                ++reader2;
            }

//			cache.flush();
//			block_type *block = new block_type;
//			unsigned i = 0;
//			for (unsigned i_block = 0; i_block < n_blocks; i_block++) {
//				stxxl::request_ptr req = block->read(bids[i_block]);
//				req->wait();
//
//				for (unsigned inner = 0; inner < block_size*subblock_size; ++inner) {
//					assert( (*block)[inner/subblock_size][inner%subblock_size].first == i);
//					assert( (*block)[inner/subblock_size][inner%subblock_size].second == i+1);
//					i++;
//				}
//			}
//			delete block;
        }

//		cache.__dump_cache();

        cache.clear();

        // finishinging subblocks: skip second half of each subblock
        {
            writer_type writer(&bids, buffer_size, buffer_size / 2);
            unsigned i = 0;
            for (unsigned outer = 0; outer < n_blocks * block_size; ++outer) {
                for (unsigned inner = 0; inner < subblock_size / 2; ++inner) {
                    writer.append(value_type(i, i));
                    ++i;
                }
                writer.finish_subblock();
            }
            writer.flush();

            reader_type reader(bids.begin(), bids.end(), &cache);
            i = 0;
            for (unsigned outer = 0; outer < n_blocks * block_size; ++outer) {
                for (unsigned inner = 0; inner < subblock_size / 2; ++inner) {
                    assert(reader.const_value().first == i);
                    ++i;
                    ++reader;
                }
                reader.next_subblock();
            }
        }

        STXXL_MSG("Passed Reader-Writer Test");
    }
    catch (...) {
        STXXL_MSG("Cought unknown exception.");
    }
}


int main()
{
    reader_writer_test();
    return 0;
}
