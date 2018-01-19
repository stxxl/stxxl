/***************************************************************************
 *  tests/algo/test_parallel_sort.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example algo/test_parallel_sort.cpp
//! This is an example of how to use the parallelized sorting algorithm.
//! Setting all the parameters in optional, just compiling with parallel mode
//! suffices.

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#if !defined(STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD)
#define STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD 0
#endif

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <random>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/defines.h>
#include <stxxl/scan>
#include <stxxl/sort>
#include <stxxl/stream>
#include <stxxl/vector>

#include <key_with_padding.h>

const unsigned long long megabyte = 1024 * 1024;
#define MAGIC 123

using KeyType = uint64_t;
constexpr size_t RecordSize = 2 * sizeof(KeyType) + 0;
using my_type = key_with_padding<KeyType, RecordSize, false>;
using cmp_less_key = my_type::compare_less;

size_t run_size;
size_t buffer_size;
constexpr int block_size = STXXL_DEFAULT_BLOCK_SIZE(my_type);

using vector_type = stxxl::vector<my_type, 4, stxxl::lru_pager<8>, block_size, foxxll::default_alloc_strategy>;

my_type::key_type checksum(vector_type& input)
{
    my_type::key_type sum = 0;

    for (const auto& v : input)
        sum += v.key;

    return sum;
}

void linear_sort_normal(vector_type& input)
{
    my_type::key_type sum1 = checksum(input);

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());
    double start = foxxll::timestamp();

    stxxl::sort(input.begin(), input.end(), cmp_less_key(), run_size);

    double stop = foxxll::timestamp();
    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    my_type::key_type sum2 = checksum(input);

    std::cout << sum1 << " ?= " << sum2 << std::endl;

    die_unless(stxxl::is_sorted(input.cbegin(), input.cend()));

    std::cout << "Linear sorting normal took " << (stop - start) << " seconds." << std::endl;
}

void linear_sort_streamed(vector_type& input, vector_type& output)
{
    my_type::key_type sum1 = checksum(input);

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());
    double start = foxxll::timestamp();

    using input_stream_type = stxxl::stream::streamify_traits<vector_type::iterator>::stream_type;

    input_stream_type input_stream = stxxl::stream::streamify(input.begin(), input.end());

    using sort_stream_type = stxxl::stream::sort<input_stream_type, cmp_less_key, block_size>;

    sort_stream_type sort_stream(input_stream, cmp_less_key(), run_size);

    vector_type::iterator o = stxxl::stream::materialize(sort_stream, output.begin(), output.end());
    die_unless(o == output.end());

    double stop = foxxll::timestamp();
    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    my_type::key_type sum2 = checksum(output);

    std::cout << sum1 << " ?= " << sum2 << std::endl;
    if (sum1 != sum2)
        LOG1 << "WRONG DATA";

    die_unless(stxxl::is_sorted(output.cbegin(), output.cend(), cmp_less_key()));

    std::cout << "Linear sorting streamed took " << (stop - start) << " seconds." << std::endl;
}

int main(int argc, const char** argv)
{
    if (argc < 6) {
        std::cout << "Usage: " << argv[0] << " [n in MiB] [p threads] [M in MiB] [sorting algorithm: m | q | qb | s] [merging algorithm: p | s | n]" << std::endl;
        return -1;
    }

    foxxll::config::get_instance();

#if STXXL_PARALLEL_MULTIWAY_MERGE
    LOG1 << "STXXL_PARALLEL_MULTIWAY_MERGE";
#endif
    unsigned long megabytes_to_process = atoi(argv[1]);
    int p = atoi(argv[2]);
    size_t memory_to_use = (size_t)(atoi(argv[3]) * megabyte);
    run_size = memory_to_use;
    buffer_size = memory_to_use / 16;
#ifdef STXXL_PARALLEL_MODE
    omp_set_num_threads(p);
    __gnu_parallel::_Settings parallel_settings(__gnu_parallel::_Settings::get());

    parallel_settings.merge_splitting = __gnu_parallel::EXACT;
    parallel_settings.merge_minimal_n = 10000;
    parallel_settings.merge_oversampling = 10;

    parallel_settings.multiway_merge_algorithm = __gnu_parallel::LOSER_TREE;
    parallel_settings.multiway_merge_splitting = __gnu_parallel::EXACT;
    parallel_settings.multiway_merge_oversampling = 10;
    parallel_settings.multiway_merge_minimal_n = 10000;
    parallel_settings.multiway_merge_minimal_k = 2;
    if (!strcmp(argv[4], "q"))                  //quicksort
        parallel_settings.sort_algorithm = __gnu_parallel::QS;
    else if (!strcmp(argv[4], "qb"))            //balanced quicksort
        parallel_settings.sort_algorithm = __gnu_parallel::QS_BALANCED;
    else if (!strcmp(argv[4], "m"))             //merge sort
        parallel_settings.sort_algorithm = __gnu_parallel::MWMS;
    else /*if(!strcmp(argv[4], "s"))*/          //sequential (default)
    {
        parallel_settings.sort_algorithm = __gnu_parallel::QS;
        parallel_settings.sort_minimal_n = memory_to_use;
    }

    if (!strcmp(argv[5], "p"))          //parallel
    {
        stxxl::SETTINGS::native_merge = false;
        //parallel_settings.multiway_merge_minimal_n = 1024;    //leave as default
    }
    else if (!strcmp(argv[5], "s"))                                             //sequential
    {
        stxxl::SETTINGS::native_merge = false;
        parallel_settings.multiway_merge_minimal_n = memory_to_use;             //too much to be called
    }
    else /*if(!strcmp(argv[5], "n"))*/                                          //native (default)
        stxxl::SETTINGS::native_merge = true;

    parallel_settings.multiway_merge_minimal_k = 2;

    __gnu_parallel::_Settings::set(parallel_settings);
    die_unless(&__gnu_parallel::_Settings::get() != &parallel_settings);

    if (0)
        printf("%d %p: mwms %d, q %d, qb %d",
               __gnu_parallel::_Settings::get().sort_algorithm,
               (void*)&__gnu_parallel::_Settings::get().sort_algorithm,
               __gnu_parallel::MWMS,
               __gnu_parallel::QS,
               __gnu_parallel::QS_BALANCED);
#endif

    std::cout << "Sorting " << megabytes_to_process << " MiB of data ("
              << (megabytes_to_process * megabyte / sizeof(my_type)) << " elements) using "
              << (memory_to_use / megabyte) << " MiB of internal memory and "
              << p << " thread(s), block size "
              << block_size << ", element size " << sizeof(my_type) << std::endl;

    const uint64_t n_records =
        uint64_t(megabytes_to_process) * uint64_t(megabyte) / sizeof(my_type);
    vector_type input(n_records);

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());
    double generate_start = foxxll::timestamp();

    {
        std::mt19937_64 randgen(p);
        std::uniform_int_distribution<KeyType> distr;
        stxxl::generate(input.begin(), input.end(),
                        [&]() -> my_type { return my_type(distr(randgen)); },
                        memory_to_use / STXXL_DEFAULT_BLOCK_SIZE(my_type));
    }

    double generate_stop = foxxll::timestamp();
    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    std::cout << "Generating took " << (generate_stop - generate_start) << " seconds." << std::endl;

    die_unless(!stxxl::is_sorted(input.cbegin(), input.cend()));

    {
        vector_type output(n_records);

        linear_sort_streamed(input, output);
        linear_sort_normal(input);
    }

    return 0;
}
// vim: et:ts=4:sw=4
