/***************************************************************************
 *  include/stxxl/bits/parallel/settings.h
 *
 *  Settings and tuning parameters, heuristics to decide whether to use
 *  parallelized algorithms.
 *  Extracted from MCSTL http://algo2.iti.uni-karlsruhe.de/singler/mcstl/
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_SETTINGS_HEADER
#define STXXL_PARALLEL_SETTINGS_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/parallel/types.h>

#include <omp.h>

STXXL_BEGIN_NAMESPACE

namespace parallel {

/** The extensible condition on whether the parallel variant of an algorithm sould be called.
 * \param c A condition that is overruled by mcstl::Settings::force_parallel,
 * i. e. usually a decision based on the input size. */
#define MCSTL_PARALLEL_CONDITION(c) (!(SETTINGS::force_sequential) && ((SETTINGS::num_threads > 1 && (c)) || SETTINGS::force_parallel))

/** Type to be passed to algorithm calls in order to guide their level of parallelism.
 *
 *  Constructing new instances should be avoided, use the existing ones in mcstl::Settings. */
struct parallelism
{
    enum Parallelism {
        /** sequential */
        SEQUENTIAL,
        /** parallel, unspecified method */ PARALLEL,
        /** parallel unbalanced (equal-sized chunks) */ PARALLEL_UNBALANCED,
        /** parallel balanced (work-stealing) */ PARALLEL_BALANCED,
        /** parallel OMP loop */ PARALLEL_OMP_LOOP,
        /** parallel OMP loop with static scheduling */ PARALLEL_OMP_LOOP_STATIC,
        /** enum end marker */ PARALLELISM_LAST
    };

    Parallelism par;

    parallelism(Parallelism _par) : par(_par) { }

    bool is_parallel() { return par != SEQUENTIAL; }
};

typedef parallelism PARALLELISM;

/** Pseudo-integer that gets its initial value from \c omp_get_max_threads(). */
class NumberOfThreads
{
public:
    int num_threads;

    NumberOfThreads()
    {
        num_threads = omp_get_max_threads();
        if (num_threads < 1)
            num_threads = 1;
    }

    operator int ()
    {
        return num_threads;
    }

    int operator = (int nt)
    {
        num_threads = nt;
        return num_threads;
    }
};

/** Run-time settings for the MCSTL.
 * \param must_be_int This template parameter exists only to avoid having an
 * object file belonging to the library. It should always be set to int,
 * to ensure deterministic behavior. */
template <typename must_be_int = int>
struct Settings
{
public:
    /** Different parallel sorting algorithms to choose from: multi-way mergesort, quicksort, load-balanced quicksort. */
    enum SortAlgorithm { MWMS, QS, QS_BALANCED };
    /** Different merging algorithms: bubblesort-alike, loser-tree variants, enum sentinel */
    enum MultiwayMergeAlgorithm { BUBBLE, LOSER_TREE_EXPLICIT, LOSER_TREE, LOSER_TREE_COMBINED, LOSER_TREE_SENTINEL, MWM_ALGORITHM_LAST };
    /** Different splitting strategies for sorting/merging: by sampling, exact */
    enum Splitting { SAMPLING, EXACT };
    /** Different partial sum algorithms: recursive, linear */
    enum PartialSumAlgorithm { RECURSIVE, LINEAR };
    /** Different find distribution strategies: growing blocks, equal-sized blocks, equal splitting. */
    enum FindDistribution { GROWING_BLOCKS, CONSTANT_SIZE_BLOCKS, EQUAL_SPLIT };

    /** Number of thread to be used.
     * Initialized to omp_get_max_threads(), but can be overridden by the user.
     */
    static NumberOfThreads num_threads;

    /** Force all algorithms to be executed sequentially.
     * This setting cannot be overwritten. */
    static volatile bool force_sequential;

    /** Force all algorithms to be executed in parallel.
     * This setting can be overriden by
     * mcstl::sequential_tag (compile-time), and
     * force_sequential (run-time). */
    static volatile bool force_parallel;

    /** Algorithm to use for sorting. */
    static volatile SortAlgorithm sort_algorithm;

    /** Strategy to use for splitting the input when sorting (MWMS). */
    static volatile Splitting sort_splitting;

    /** Minimal input size for parallel sorting. */
    static volatile sequence_index_t sort_minimal_n;
    /** Oversampling factor for parallel std::sort (MWMS). */
    static volatile unsigned int sort_mwms_oversampling;
    /** Such many samples to take to find a good pivot (quicksort). */
    static volatile unsigned int sort_qs_num_samples_preset;

    /** Maximal subsequence length to swtich to unbalanced base case.
     * Applies to std::sort with dynamically load-balanced quicksort. */
    static volatile sequence_index_t sort_qsb_base_case_maximal_n;

    /** Minimal input size for parallel std::partition. */
    static volatile sequence_index_t partition_minimal_n;
    /** Chunk size for parallel std::partition. */
    static volatile sequence_index_t partition_chunk_size;
    /** Chunk size for parallel std::partition, relative to input size.
     * If >0.0, this value overrides partition_chunk_size. */
    static volatile double partition_chunk_share;

    /** Minimal input size for parallel std::nth_element. */
    static volatile sequence_index_t nth_element_minimal_n;

    /** Minimal input size for parallel std::partial_sort. */
    static volatile sequence_index_t partial_sort_minimal_n;

    /** Minimal input size for parallel std::adjacent_difference. */
    static volatile unsigned int adjacent_difference_minimal_n;

    /** Minimal input size for parallel std::partial_sum. */
    static volatile unsigned int partial_sum_minimal_n;
    /** Algorithm to use for std::partial_sum. */
    static volatile PartialSumAlgorithm partial_sum_algorithm;
    /** Assume "sum and write result" to be that factor slower than just "sum".
     *  This value is used for std::partial_sum. */
    static volatile float partial_sum_dilatation;

    /** Minimal input size for parallel std::random_shuffle. */
    static volatile unsigned int random_shuffle_minimal_n;

    /** Minimal input size for parallel std::merge. */
    static volatile sequence_index_t merge_minimal_n;
    /** Splitting strategy for parallel std::merge. */
    static volatile Splitting merge_splitting;
    /** Oversampling factor for parallel std::merge.
     * Such many samples per thread are collected. */
    static volatile unsigned int merge_oversampling;

    /** Algorithm to use for parallel mcstl::multiway_merge. */
    static volatile MultiwayMergeAlgorithm multiway_merge_algorithm;
    /** Splitting strategy to use for parallel mcstl::multiway_merge. */
    static volatile Splitting multiway_merge_splitting;
    /** Oversampling factor for parallel mcstl::multiway_merge. */
    static volatile unsigned int multiway_merge_oversampling;
    /** Minimal input size for parallel mcstl::multiway_merge. */
    static volatile sequence_index_t multiway_merge_minimal_n;
    /** Oversampling factor for parallel mcstl::multiway_merge. */
    static volatile int multiway_merge_minimal_k;

    /** Minimal input size for parallel std::unique_copy. */
    static volatile sequence_index_t unique_copy_minimal_n;

    static volatile sequence_index_t workstealing_chunk_size;
    /** Minimal input size for parallel std::for_each. */
    static volatile sequence_index_t for_each_minimal_n;
    /** Minimal input size for parallel std::count and std::count_if. */
    static volatile sequence_index_t count_minimal_n;
    /** Minimal input size for parallel std::transform. */
    static volatile sequence_index_t transform_minimal_n;
    /** Minimal input size for parallel std::replace and std::replace_if. */
    static volatile sequence_index_t replace_minimal_n;
    /** Minimal input size for parallel std::generate. */
    static volatile sequence_index_t generate_minimal_n;

    /** Minimal input size for parallel std::fill. */
    static volatile sequence_index_t fill_minimal_n;

    /** Minimal input size for parallel std::min_element. */
    static volatile sequence_index_t min_element_minimal_n;
    /** Minimal input size for parallel std::max_element. */
    static volatile sequence_index_t max_element_minimal_n;
    /** Minimal input size for parallel std::accumulate. */
    static volatile sequence_index_t accumulate_minimal_n;

    /** Distribution strategy for parallel std::find. */
    static volatile FindDistribution find_distribution;
    /** Start with looking for that many elements sequentially, for std::find. */
    static volatile sequence_index_t find_sequential_search_size;
    /** Initial block size for parallel std::find. */
    static volatile sequence_index_t find_initial_block_size;
    /** Maximal block size for parallel std::find. */
    static volatile sequence_index_t find_maximum_block_size;
    /** Block size increase factor for parallel std::find. */
    static volatile double find_increasing_factor;

//set operations
    /** Minimal input size for parallel std::set_union. */
    static volatile sequence_index_t set_union_minimal_n;
    /** Minimal input size for parallel std::set_symmetric_difference. */
    static volatile sequence_index_t set_symmetric_difference_minimal_n;
    /** Minimal input size for parallel std::set_difference. */
    static volatile sequence_index_t set_difference_minimal_n;
    /** Minimal input size for parallel std::set_intersection. */
    static volatile sequence_index_t set_intersection_minimal_n;

//hardware dependent tuning parameters
    /** Size of the L1 cache in bytes (underestimation). */
    static volatile unsigned long long L1_cache_size;
    /** Size of the L2 cache in bytes (underestimation). */
    static volatile unsigned long long L2_cache_size;
    /** Size of the Translation Lookaside Buffer (underestimation). */
    static volatile unsigned int TLB_size;

    /** Overestimation of cache line size.
     * Used to avoid false sharing, i. e. elements of different threads are at least this amount apart. */
    static unsigned int cache_line_size;        //overestimation

    /** Parallelization tag: recommend sequential execution. */
    static const parallelism sequential;
    /** Parallelization tag: recommend general parallel execution. */
    static const parallelism parallel;
    /** Parallelization tag: recommend parallel execution with static dynamic load-balancing. */
    static const parallelism parallel_unbalanced;
    /** Parallelization tag: recommend parallel execution with dynamic load-balancing. */
    static const parallelism parallel_balanced;
    /** Parallelization tag: recommend parallel execution with OpenMP dynamic load-balancing. */
    static const parallelism parallel_omp_loop;
    /** Parallelization tag: recommend parallel execution with OpenMP static load-balancing. */
    static const parallelism parallel_omp_loop_static;
    /** Parallelization tag: recommend parallel execution using OpenMP taskqueue construct. */
    static const parallelism parallel_taskqueue;

    //statistics

    /** Statistic on the number of stolen ranges in load-balanced quicksort.*/
    static volatile sequence_index_t qsb_steals;
};

/** Convenience typedef to avoid to have write \c Settings<>. */
typedef Settings<> SETTINGS;

/** Convenience typedef to avoid to have write \c Settings<>.
 *
 *  \deprecated For backward compatibility only. */
typedef Settings<> HEURISTIC;

template <typename must_be_int>
volatile bool Settings<must_be_int>::force_parallel = false;

template <typename must_be_int>
volatile bool Settings<must_be_int>::force_sequential = false;

template <typename must_be_int>
volatile typename Settings<must_be_int>::SortAlgorithm Settings<must_be_int>::sort_algorithm = Settings<must_be_int>::MWMS;

template <typename must_be_int>
volatile typename Settings<must_be_int>::Splitting Settings<must_be_int>::sort_splitting = Settings<must_be_int>::EXACT;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::sort_minimal_n = 1000;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::sort_mwms_oversampling = 10;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::sort_qs_num_samples_preset = 100;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::sort_qsb_base_case_maximal_n = 100;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::partition_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::nth_element_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::partial_sort_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::partition_chunk_size = 1000;

template <typename must_be_int>
volatile double Settings<must_be_int>::partition_chunk_share = 0.0;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::adjacent_difference_minimal_n = 1000;

template <typename must_be_int>
volatile typename Settings<must_be_int>::PartialSumAlgorithm Settings<must_be_int>::partial_sum_algorithm = Settings<must_be_int>::LINEAR;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::partial_sum_minimal_n = 1000;

template <typename must_be_int>
volatile float Settings<must_be_int>::partial_sum_dilatation = 1.0f;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::random_shuffle_minimal_n = 1000;

template <typename must_be_int>
volatile typename Settings<must_be_int>::Splitting Settings<must_be_int>::merge_splitting = Settings<must_be_int>::EXACT;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::merge_minimal_n = 1000;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::merge_oversampling = 10;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::multiway_merge_minimal_n = 1000;

template <typename must_be_int>
volatile int Settings<must_be_int>::multiway_merge_minimal_k = 2;

// unique copy
template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::unique_copy_minimal_n = 10000;

template <typename must_be_int>
volatile typename Settings<must_be_int>::MultiwayMergeAlgorithm Settings<must_be_int>::multiway_merge_algorithm = Settings<must_be_int>::LOSER_TREE;

template <typename must_be_int>
volatile typename Settings<must_be_int>::Splitting Settings<must_be_int>::multiway_merge_splitting = Settings<must_be_int>::EXACT;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::multiway_merge_oversampling = 10;

template <typename must_be_int>
volatile typename Settings<must_be_int>::FindDistribution Settings<must_be_int>::find_distribution = Settings<must_be_int>::CONSTANT_SIZE_BLOCKS;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::find_sequential_search_size = 256;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::find_initial_block_size = 256;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::find_maximum_block_size = 8192;

template <typename must_be_int>
volatile double Settings<must_be_int>::find_increasing_factor = 2.0;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::workstealing_chunk_size = 100;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::for_each_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::count_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::transform_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::replace_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::generate_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::fill_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::min_element_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::max_element_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::accumulate_minimal_n = 1000;

//set operations

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::set_union_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::set_intersection_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::set_difference_minimal_n = 1000;

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::set_symmetric_difference_minimal_n = 1000;

template <typename must_be_int>
volatile unsigned long long Settings<must_be_int>::L1_cache_size = 16 << 10;

template <typename must_be_int>
volatile unsigned long long Settings<must_be_int>::L2_cache_size = 256 << 10;

template <typename must_be_int>
volatile unsigned int Settings<must_be_int>::TLB_size = 128;

template <typename must_be_int>
unsigned int Settings<must_be_int>::cache_line_size = 64;       //overestimation

template <typename must_be_int>
const parallelism Settings<must_be_int>::sequential(parallelism::SEQUENTIAL);

template <typename must_be_int>
const parallelism Settings<must_be_int>::parallel(parallelism::PARALLEL);

template <typename must_be_int>
const parallelism Settings<must_be_int>::parallel_unbalanced(parallelism::PARALLEL_UNBALANCED);

template <typename must_be_int>
const parallelism Settings<must_be_int>::parallel_balanced(parallelism::PARALLEL_BALANCED);

template <typename must_be_int>
const parallelism Settings<must_be_int>::parallel_omp_loop(parallelism::PARALLEL_OMP_LOOP);

template <typename must_be_int>
const parallelism Settings<must_be_int>::parallel_omp_loop_static(parallelism::PARALLEL_OMP_LOOP_STATIC);

template <typename must_be_int>
NumberOfThreads Settings<must_be_int>::num_threads;

//statistics

template <typename must_be_int>
volatile sequence_index_t Settings<must_be_int>::qsb_steals = 0;

} // namespace parallel

STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_SETTINGS_HEADER
