/***************************************************************************
 *  tests/algo/test_sort_all_parameters.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
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
#include <stxxl/scan>
#include <stxxl/sort>
#include <stxxl/vector>

#include <key_with_padding.h>

#ifndef RECORD_SIZE
 #define RECORD_SIZE 4
#endif

constexpr size_t MB = 1024 * 1024;

template <typename T, typename alloc_strategy_type, unsigned block_size>
void test(uint64_t data_mem, size_t memory_to_use)
{
    uint64_t records_to_sort = data_mem / sizeof(T);
    using vector_type = stxxl::vector<T, 2, stxxl::lru_pager<8>, block_size, alloc_strategy_type>;
    vector_type v(records_to_sort);

    using cmp = typename T::compare_less;

    size_t ndisks = foxxll::config::get_instance()->disks_number();
    LOG1 << "Sorting " << records_to_sort << " records of size " << sizeof(T);
    LOG1 << "Total volume " << (records_to_sort * sizeof(T)) / MB << " MiB";
    LOG1 << "Using " << memory_to_use / MB << " MiB";
    LOG1 << "Using " << ndisks << " disks";
    LOG1 << "Using " << alloc_strategy_type::name() << " allocation strategy ";
    LOG1 << "Block size " << vector_type::block_type::raw_size / 1024 << " KiB";

    LOG1 << "Filling vector...";
    stxxl::generate(v.begin(), v.end(), stxxl::random_number32_r(), 32);

    LOG1 << "Sorting vector...";

    foxxll::stats_data before(*foxxll::stats::get_instance());

    stxxl::sort(v.begin(), v.end(), cmp(), memory_to_use);

    foxxll::stats_data after(*foxxll::stats::get_instance());

    LOG1 << "Checking order...";
    die_unless(stxxl::is_sorted(v.cbegin(), v.cend(), cmp()));

    LOG1 << "Sorting: " << (after - before);
    LOG1 << "Total:   " << *foxxll::stats::get_instance();
}

template <typename T, unsigned block_size>
void test_all_strategies(
    uint64_t data_mem,
    size_t memory_to_use,
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
    }

#if STXXL_PARALLEL_MULTIWAY_MERGE
    LOG1 << "STXXL_PARALLEL_MULTIWAY_MERGE";
#endif
    uint64_t data_mem = foxxll::atouint64(argv[1]) * MB;
    size_t sort_mem = strtoul(argv[2], nullptr, 0) * MB;
    int strategy = atoi(argv[3]);
    int block_size_switch = atoi(argv[4]);
    stxxl::set_seed((unsigned)strtoul(argv[5], nullptr, 10));
    LOG1 << "Seed " << stxxl::get_next_seed();
    stxxl::srandom_number32();

    using my_default_type = key_with_padding<unsigned, RECORD_SIZE>;

    switch (block_size_switch)
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
        test_all_strategies<key_with_padding<unsigned, 128>, 2* MB>(data_mem, sort_mem, strategy);
        break;
    default:
        LOG1 << "Unknown block size: " << block_size_switch << ", aborting";
        abort();
    }

    return 0;
}
