/***************************************************************************
 *  tests/parallel/bench_multiway_merge.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *  Copyright (C) 2014-2015 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <vector>
#include <utility>
#include <omp.h>
#include <cstdlib>
#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/common/rand.h>
#include <stxxl/bits/common/is_sorted.h>
#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/bits/parallel/multiway_merge.h>

unsigned int g_repeat = 3;

struct DataStruct
{
    unsigned int key;
    char payload[32];

    explicit DataStruct(unsigned int k = 0)
        : key(k)
    { }

    bool operator < (const DataStruct& other) const
    {
        return key < other.key;
    }

    bool operator == (const DataStruct& other) const
    {
        return (key == other.key);
    }

    friend std::ostream& operator << (std::ostream& os, const DataStruct& s)
    {
        return os << '(' << s.key << ",...)";
    }
};

enum benchmark_type {
    SEQ_MWM_LT,
    SEQ_MWM_LT_STABLE,
    SEQ_MWM_LT_COMBINED,
    SEQ_GNU_MWM,

    PARA_MWM_EXACT_LT,
    PARA_MWM_EXACT_LT_STABLE,
    PARA_MWM_SAMPLING_LT,
    PARA_MWM_SAMPLING_LT_STABLE,
    PARA_GNU_MWM_EXACT,
    PARA_GNU_MWM_SAMPLING
};

template <typename ValueType, benchmark_type Method>
void test_multiway_merge(unsigned int seqnum)
{
    // we allocate a list of blocks, each block being a sequence of items.
    static const size_t value_size = sizeof(ValueType);
    static const size_t block_bytes = 2 * 1024 * 1024;
    static const size_t block_size = block_bytes / value_size;

    typedef std::vector<ValueType> sequence_type;

    size_t total_size = seqnum * block_size;
    size_t total_bytes = seqnum * block_bytes;

    std::less<ValueType> cmp;

    std::vector<sequence_type> blocks(seqnum);

    {
        stxxl::scoped_print_timer spt("Filling blocks with random numbers in parallel", total_bytes);

        for (size_t i = 0; i < seqnum; ++i)
            blocks[i].resize(block_size);

#pragma omp parallel
        {
            unsigned int seed = 1234 * omp_get_thread_num();
            stxxl::random_number32_r rnd(seed);
#pragma omp for
            for (size_t i = 0; i < seqnum; ++i)
            {
                for (size_t j = 0; j < block_size; ++j)
                    blocks[i][j] = ValueType(rnd());

                std::sort(blocks[i].begin(), blocks[i].end(), cmp);
            }
        }
    }

    std::vector<ValueType> out;

    {
        stxxl::scoped_print_timer spt("Allocating output buffer", total_bytes);
        out.resize(total_size);
    }

    {
        stxxl::scoped_print_timer spt("Merging", total_bytes);

        const char* method_name = NULL;

        typedef std::pair<typename sequence_type::iterator,
                          typename sequence_type::iterator>
            sequence_iterator_pair_type;
        std::vector<sequence_iterator_pair_type> sequences(seqnum);

        for (size_t i = 0; i < seqnum; ++i) {
            sequences[i] =
                sequence_iterator_pair_type(blocks[i].begin(), blocks[i].end());
        }

        using stxxl::parallel::SETTINGS;

        switch (Method)
        {
        case SEQ_MWM_LT:
            method_name = "seq_mwm_lt";

            SETTINGS::multiway_merge_algorithm = SETTINGS::LOSER_TREE;

            stxxl::parallel::sequential_multiway_merge<false, false>(
                sequences.begin(), sequences.end(),
                out.begin(), total_size, cmp);
            break;

        case SEQ_MWM_LT_STABLE:
            method_name = "seq_mwm_lt_stable";

            SETTINGS::multiway_merge_algorithm = SETTINGS::LOSER_TREE;

            stxxl::parallel::sequential_multiway_merge<true, false>(
                sequences.begin(), sequences.end(),
                out.begin(), total_size, cmp);
            break;

        case SEQ_MWM_LT_COMBINED:
            method_name = "seq_mwm_lt_combined";

            SETTINGS::multiway_merge_algorithm = SETTINGS::LOSER_TREE_COMBINED;

            stxxl::parallel::sequential_multiway_merge<false, false>(
                sequences.begin(), sequences.end(),
                out.begin(), total_size, cmp);
            break;

        case SEQ_GNU_MWM:
            method_name = "seq_gnu_mwm";

            __gnu_parallel::multiway_merge(sequences.begin(), sequences.end(),
                                           out.begin(), total_size, cmp,
                                           __gnu_parallel::sequential_tag());
            break;

        case PARA_MWM_EXACT_LT:
            method_name = "para_mwm_exact_lt";

            SETTINGS::multiway_merge_algorithm = SETTINGS::LOSER_TREE;
            SETTINGS::multiway_merge_splitting = SETTINGS::EXACT;

            stxxl::parallel::multiway_merge<false>(
                sequences.begin(), sequences.end(),
                out.begin(), total_size, cmp);
            break;

        case PARA_MWM_EXACT_LT_STABLE:
            method_name = "para_mwm_exact_lt_stable";

            SETTINGS::multiway_merge_algorithm = SETTINGS::LOSER_TREE;
            SETTINGS::multiway_merge_splitting = SETTINGS::EXACT;

            stxxl::parallel::multiway_merge<true>(
                sequences.begin(), sequences.end(),
                out.begin(), total_size, cmp);
            break;

        case PARA_MWM_SAMPLING_LT:
            method_name = "para_mwm_sampling_lt";

            SETTINGS::multiway_merge_algorithm = SETTINGS::LOSER_TREE;
            SETTINGS::multiway_merge_splitting = SETTINGS::SAMPLING;

            stxxl::parallel::multiway_merge<false>(
                sequences.begin(), sequences.end(),
                out.begin(), total_size, cmp);
            break;

        case PARA_MWM_SAMPLING_LT_STABLE:
            method_name = "para_mwm_sampling_lt_stable";

            SETTINGS::multiway_merge_algorithm = SETTINGS::LOSER_TREE;
            SETTINGS::multiway_merge_splitting = SETTINGS::SAMPLING;

            stxxl::parallel::multiway_merge<true>(
                sequences.begin(), sequences.end(),
                out.begin(), total_size, cmp);
            break;

        case PARA_GNU_MWM_EXACT: {
            method_name = "para_gnu_mwm_exact";

            __gnu_parallel::_Settings s = __gnu_parallel::_Settings::get();
            s.multiway_merge_splitting = __gnu_parallel::EXACT;
            __gnu_parallel::_Settings::set(s);

            __gnu_parallel::multiway_merge(sequences.begin(), sequences.end(),
                                           out.begin(), total_size, cmp);
            break;
        }

        case PARA_GNU_MWM_SAMPLING: {
            method_name = "para_gnu_mwm_sampling";

            __gnu_parallel::_Settings s = __gnu_parallel::_Settings::get();
            s.multiway_merge_splitting = __gnu_parallel::SAMPLING;
            __gnu_parallel::_Settings::set(s);

            __gnu_parallel::multiway_merge(sequences.begin(), sequences.end(),
                                           out.begin(), total_size, cmp);
            break;
        }
        }

        std::cout << "RESULT"
                  << " seqnum=" << seqnum
                  << " method=" << method_name
                  << " value_size=" << value_size
                  << " block_size=" << block_size
                  << " total_size=" << total_size
                  << " total_bytes=" << total_bytes
                  << " num_threads=" << omp_get_max_threads()
                  << " time=" << spt.timer().seconds()
                  << "\n";
    }

    STXXL_CHECK(stxxl::is_sorted(out.begin(), out.end(), cmp));
}

template <typename ValueType, benchmark_type Method>
void test_repeat(unsigned int seqnum)
{
    for (unsigned int r = 0; r < g_repeat; ++r)
        test_multiway_merge<ValueType, Method>(seqnum);
}

template <typename ValueType, benchmark_type Method>
void test_all_vecnum()
{
    static const unsigned int factor = 32;

    for (unsigned int s = 1; s < 128; s += 1 * factor)
        test_repeat<ValueType, Method>(s);

    for (unsigned int s = 128; s < 256; s += 4 * factor)
        test_repeat<ValueType, Method>(s);

    for (unsigned int s = 256; s < 512; s += 16 * factor)
        test_repeat<ValueType, Method>(s);

    for (unsigned int s = 512; s < 1024; s += 64 * factor)
        test_repeat<ValueType, Method>(s);

    for (unsigned int s = 1024; s < 4 * 1024; s += 256 * factor)
        test_repeat<ValueType, Method>(s);
}

template <typename ValueType>
void test_sequential()
{
    test_all_vecnum<ValueType, SEQ_MWM_LT>();
    test_all_vecnum<ValueType, SEQ_MWM_LT_STABLE>();
    test_all_vecnum<ValueType, SEQ_MWM_LT_COMBINED>();
    test_all_vecnum<ValueType, SEQ_GNU_MWM>();
}

template <typename ValueType>
void test_parallel()
{
    test_all_vecnum<ValueType, PARA_MWM_EXACT_LT>();
    test_all_vecnum<ValueType, PARA_MWM_EXACT_LT_STABLE>();
    test_all_vecnum<ValueType, PARA_MWM_SAMPLING_LT>();
    test_all_vecnum<ValueType, PARA_MWM_SAMPLING_LT_STABLE>();
    test_all_vecnum<ValueType, PARA_GNU_MWM_EXACT>();
    test_all_vecnum<ValueType, PARA_GNU_MWM_SAMPLING>();
}

int main(int argc, char* argv[])
{
    // run individually for debugging
    //test_repeat<uint32_t, PARA_GNU_MWM_EXACT>(32);
    //test_repeat<uint32_t, PARA_MWM_EXACT_LT>(32);
    //test_repeat<uint32_t, PARA_MWM_EXACT_LT>(1024);
    //test_repeat<uint32_t, SEQ_MWM_LT>(256);
    //test_repeat<uint32_t, SEQ_GNU_MWM>(256);
    //return 0;

    std::string benchset;

    stxxl::cmdline_parser cp;
    cp.set_description("STXXL multiway_merge benchmark");
    cp.add_param_string("seq/para/both", "benchmark set: seq(uential), para(llel) or both", benchset);

    cp.add_uint('r', "repeat", "number of repetitions", g_repeat);

    if (!cp.process(argc, argv))
        return EXIT_FAILURE;

    if (benchset == "seq" || benchset == "sequential" || benchset == "both")
    {
        test_sequential<uint32_t>();
        test_sequential<DataStruct>();
    }
    if (benchset == "para" || benchset == "parallel" || benchset == "both")
    {
        test_parallel<uint32_t>();
        test_parallel<DataStruct>();
    }

    return 0;
}
