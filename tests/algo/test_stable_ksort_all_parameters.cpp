/***************************************************************************
 *  tests/algo/test_stable_ksort_all_parameters.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//#define PLAY_WITH_OPT_PREF

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/mng.hpp>

#include <stxxl/random>
#include <stxxl/stable_ksort>
#include <stxxl/vector>

#include <key_with_padding.h>

#ifndef RECORD_SIZE
 #define RECORD_SIZE 128
#endif

constexpr size_t MB = 1024 * 1024;

template <typename T, typename alloc_strategy_type, size_t block_size>
void test(size_t data_mem, size_t memory_to_use)
{
    size_t records_to_sort = data_mem / sizeof(T);
    using vector_type = stxxl::vector<T, 2, stxxl::lru_pager<8>, block_size, alloc_strategy_type>;

    memory_to_use = foxxll::div_ceil(memory_to_use, vector_type::block_type::raw_size) * vector_type::block_type::raw_size;

    vector_type v(records_to_sort);
    size_t ndisks = foxxll::config::get_instance()->disks_number();
    LOG1 << "Sorting " << records_to_sort << " records of size " << sizeof(T);
    LOG1 << "Total volume " << (records_to_sort * sizeof(T)) / MB << " MiB";
    LOG1 << "Using " << memory_to_use / MB << " MiB";
    LOG1 << "Using " << ndisks << " disks";
    LOG1 << "Using " << alloc_strategy_type::name() << " allocation strategy ";
    LOG1 << "Block size " << vector_type::block_type::raw_size / 1024 << " KiB";

    LOG1 << "Filling vector...";
    std::generate(v.begin(), v.end(), stxxl::random_number64() _STXXL_FORCE_SEQUENTIAL);

    LOG1 << "Sorting vector...";

    foxxll::stats_data before(*foxxll::stats::get_instance());

    using get_key = typename T::key_extract;
    stxxl::stable_ksort(v.begin(), v.end(), get_key(), memory_to_use);

    foxxll::stats_data after(*foxxll::stats::get_instance());

    LOG1 << "Checking order...";
    die_unless(stxxl::is_sorted(v.cbegin(), v.cend()));

    LOG1 << "Sorting: " << (after - before);
    LOG1 << "Total:   " << *foxxll::stats::get_instance();
}

template <typename T, size_t block_size>
void test_all_strategies(
    size_t data_mem,
    unsigned memory_to_use,
    int strategy)
{
    switch (strategy)
    {
    case 0:
        test<T, foxxll::striping, block_size>(data_mem, memory_to_use);
        break;
    case 1:
        test<T, foxxll::simple_random, block_size>(data_mem, memory_to_use);
        break;
    case 2:
        test<T, foxxll::fully_random, block_size>(data_mem, memory_to_use);
        break;
    case 3:
        test<T, foxxll::random_cyclic, block_size>(data_mem, memory_to_use);
        break;
    default:
        die("Unknown allocation strategy: " << strategy << ", aborting");
    }
}

int main(int argc, char* argv[])
{
    if (argc < 6)
    {
        die("Usage: " << argv[0] <<
            " <MiB to sort> <MiB to use> <alloc_strategy [0..3]> <blk_size [0..14]> <seed>");
        return -1;
    }

#if STXXL_PARALLEL_MULTIWAY_MERGE
    LOG1 << "STXXL_PARALLEL_MULTIWAY_MERGE";
#endif
    size_t data_mem = static_cast<size_t>(atoi(argv[1])) * MB;
    int sort_mem = atoi(argv[2]) * MB;
    int strategy = atoi(argv[3]);
    int block_size = atoi(argv[4]);
    stxxl::set_seed((unsigned)strtoul(argv[5], nullptr, 10));
    LOG1 << "Seed " << stxxl::get_next_seed();
    stxxl::srandom_number32();

    using my_default_type = key_with_padding<uint64_t, RECORD_SIZE>;

    switch (block_size)
    {
    case 0:
        test_all_strategies<my_default_type, 128* 1024>(data_mem, sort_mem, strategy);
        break;
    case 1:
        test_all_strategies<my_default_type, 256* 1024>(data_mem, sort_mem, strategy);
        break;
    case 2:
        test_all_strategies<my_default_type, 512* 1024>(data_mem, sort_mem, strategy);
        break;
    case 3:
        test_all_strategies<my_default_type, 1024* 1024>(data_mem, sort_mem, strategy);
        break;
    case 4:
        test_all_strategies<my_default_type, 2* 1024* 1024>(data_mem, sort_mem, strategy);
        break;
    case 5:
        test_all_strategies<my_default_type, 4* 1024* 1024>(data_mem, sort_mem, strategy);
        break;
    case 6:
        test_all_strategies<my_default_type, 8* 1024* 1024>(data_mem, sort_mem, strategy);
        break;
    case 7:
        test_all_strategies<my_default_type, 16* 1024* 1024>(data_mem, sort_mem, strategy);
        break;
    case 8:
        test_all_strategies<my_default_type, 640* 1024>(data_mem, sort_mem, strategy);
        break;
    case 9:
        test_all_strategies<my_default_type, 768* 1024>(data_mem, sort_mem, strategy);
        break;
    case 10:
        test_all_strategies<my_default_type, 896* 1024>(data_mem, sort_mem, strategy);
        break;
    case 11:
        test_all_strategies<key_with_padding<unsigned, 12>, 2* MB>(data_mem, sort_mem, strategy);
        break;
    case 12:
        test_all_strategies<key_with_padding<unsigned, 12>, 2* MB + 4096>(data_mem, sort_mem, strategy);
        break;
    case 13:
        test_all_strategies<key_with_padding<unsigned, 20>, 2* MB + 4096>(data_mem, sort_mem, strategy);
        break;
    case 14:
        test_all_strategies<key_with_padding<unsigned, 8>, 2* MB>(data_mem, sort_mem, strategy);
        break;
    default:
        die("Unknown block size: " << block_size << ", aborting");
    }

    return 0;
}
