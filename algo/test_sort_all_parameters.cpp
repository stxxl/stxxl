/***************************************************************************
 *  algo/test_sort_all_parameters.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include "stxxl/mng"
#include "stxxl/sort"
#include "stxxl/scan"
#include "stxxl/vector"
#include "stxxl/random"


//#define COUNT_WAIT_TIME

//#define PLAY_WITH_OPT_PREF

int n_prefetch_buffers;
int n_opt_prefetch_buffers;


#ifndef RECORD_SIZE
 #define RECORD_SIZE 4
#endif

struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    // char _data[RECORD_SIZE - sizeof(key_type)];

    my_type() { }
    my_type(key_type __key) : _key(__key) { }
};

std::ostream & operator << (std::ostream & o, const my_type obj)
{
    o << obj._key;
    return o;
}


bool operator < (const my_type & a, const my_type & b)
{
    return a._key < b._key;
}

bool operator != (const my_type & a, const my_type & b)
{
    return a._key != b._key;
}

struct Cmp : public std::less<my_type>
{
    bool operator () (const my_type & a, const my_type & b) const
    {
        return a._key < b._key;
    }

    static my_type min_value()
    {
        return my_type(0);
    }
    static my_type max_value()
    {
        return my_type(0xffffffff);
    }
};

#define MB (1024 * 1024)


struct zero
{
    unsigned operator () ()
    {
        return 0;
    }
};

template <typename alloc_strategy_type, unsigned block_size>
void test(stxxl::int64 records_to_sort, unsigned memory_to_use, unsigned n_prefetch_blocks)
{
    typedef stxxl::vector<my_type, 2, stxxl::lru_pager<8>, block_size, alloc_strategy_type> vector_type;


    vector_type v(records_to_sort);

    unsigned ndisks = stxxl::config::get_instance()->disks_number();
    STXXL_MSG("Sorting " << records_to_sort << " records of size " << sizeof(my_type));
    STXXL_MSG("Total volume " << (records_to_sort * sizeof(my_type)) / MB << " MB");
    STXXL_MSG("Using " << memory_to_use / MB << " MB");
    STXXL_MSG("Using " << ndisks << " disks");
    STXXL_MSG("Using " << alloc_strategy_type::name() << " allocation strategy ");
    STXXL_MSG("Block size " << vector_type::block_type::raw_size / 1024 << " KB");
    STXXL_MSG("Prefetch buffers " << n_prefetch_blocks << " = " << (double(n_prefetch_blocks) / ndisks) << "*D");
    n_prefetch_buffers = n_prefetch_blocks;
    STXXL_MSG("OPT Prefetch buffers " << n_opt_prefetch_buffers << " = " << (double(n_opt_prefetch_buffers) / ndisks) << "*D");
    const int n_write_blocks = STXXL_MAX(2 * ndisks,
                                         int(memory_to_use / vector_type::block_type::raw_size) -
                                         int(2 * (records_to_sort * sizeof(my_type)) / memory_to_use) - n_prefetch_blocks);
    STXXL_MSG("Write buffers " << (n_write_blocks) << " = " << (double(n_write_blocks) / ndisks) << "*D");
    STXXL_MSG("Seed " << stxxl::ran32State);
    STXXL_MSG("Filling vector...");

    stxxl::generate(v.begin(), v.end(), stxxl::random_number32(), 32);

    stxxl::wait_time_counter = 0.0;

    STXXL_MSG("Sorting vector...");
    reset_io_wait_time();

    stxxl::sort(v.begin(), v.end(), Cmp(), memory_to_use);

    // STXXL_MSG("Checking order...");
    // STXXL_MSG( ((stxxl::is_sorted(v.begin(),v.end()))?"OK":"WRONG" ));
}

template <unsigned block_size>
void test_all_strategies(
    stxxl::int64 records_to_sort,
    unsigned memory_to_use,
    unsigned n_prefetch_blocks,
    int strategy)
{
    switch (strategy)
    {
    case 0:
        test<stxxl::striping, block_size>(records_to_sort, memory_to_use, n_prefetch_blocks);
        break;
    case 1:
        test<stxxl::SR, block_size>(records_to_sort, memory_to_use, n_prefetch_blocks);
        break;
    case 2:
        test<stxxl::FR, block_size>(records_to_sort, memory_to_use, n_prefetch_blocks);
        break;
    case 3:
        test<stxxl::RC, block_size>(records_to_sort, memory_to_use, n_prefetch_blocks);
        break;
    default:
        STXXL_ERRMSG("Unknown allocation strategy: " << strategy << ", aborting");
        abort();
    }
}

int main(int argc, char * argv[])
{
    if (argc < 8)
    {
        STXXL_ERRMSG("Usage: " << argv[0] <<
                     " <MB to sort> <MB to use> <alloc_strategy [0..3]> <blk_size [0..10]> <prefetch_buffers> <opt_pref_b> <seed>");
        return -1;
    }

    stxxl::int64 n_records = (stxxl::atoint64(argv[1]) * MB) / sizeof(my_type);
    int sort_mem = atoi(argv[2]) * MB;
    int strategy = atoi(argv[3]);
    int block_size = atoi(argv[4]);
    int n_prefetch_buffers = atoi(argv[5]);
    n_opt_prefetch_buffers = atoi(argv[6]);
    stxxl::ran32State = strtoul(argv[7], NULL, 10);

    switch (block_size)
    {
    case 0:
        test_all_strategies<(128 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 1:
        test_all_strategies<(256 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 2:
        test_all_strategies<(512 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 3:
        test_all_strategies<(1024 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 4:
        test_all_strategies<(2 * 1024 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 5:
        test_all_strategies<(4 * 1024 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 6:
        test_all_strategies<(8 * 1024 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 7:
        test_all_strategies<(16 * 1024 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 8:
        test_all_strategies<(640 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 9:
        test_all_strategies<(768 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    case 10:
        test_all_strategies<(896 * 1024)>(n_records, sort_mem, n_prefetch_buffers, strategy);
        break;
    default:
        STXXL_ERRMSG("Unknown block size: " << block_size << ", aborting");
        abort();
    }

    return 0;
}
