/***************************************************************************
 *  algo/test_parallel_sort.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define MCSTL_QUICKSORT_WORKAROUND 0

#define STXXL_PARALLEL_MULTIWAY_MERGE 1
#define STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD 0

#include <algorithm>
#include <functional>
#include <limits>

#include <stxxl/vector>
#include <stxxl/algorithm>
#include <stxxl/stream>

#ifdef __MCSTL__
#include <mcstl.h>
#endif


const unsigned long long megabyte = 1024 * 1024;

//const int block_size = STXXL_DEFAULT_BLOCK_SIZE(my_type);
const int block_size = 4 * megabyte;

#define RECORD_SIZE 128
#define MAGIC 123

stxxl::unsigned_type run_size;
stxxl::unsigned_type buffer_size;

struct my_type
{
    typedef unsigned long long key_type;

    key_type _key;
    key_type _load;
    char _data[RECORD_SIZE - 2 * sizeof(key_type)];
    key_type key() const { return _key; }

    my_type() { }
    my_type(key_type __key) : _key(__key) { }
    my_type(key_type __key, key_type __load) : _key(__key), _load(__load) { }

    void operator = (const key_type & __key) { _key = __key; }
    void operator = (const my_type & mt)
    {
        _key = mt._key;
        _load = mt._load;
    }
};

bool operator < (const my_type & a, const my_type & b);

inline bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}

inline bool operator == (const my_type & a, const my_type & b)
{
    return a.key() == b.key();
}

inline std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj._key << "/" << obj._load;
    return o;
}

struct cmp_less_key : public std::less<my_type>
{
    my_type min_value() const { return my_type(std::numeric_limits<my_type::key_type>::min(), MAGIC); }
    my_type max_value() const { return my_type(std::numeric_limits<my_type::key_type>::max(), MAGIC); }
};

typedef stxxl::vector<my_type, 4, stxxl::lru_pager<8>, block_size, STXXL_DEFAULT_ALLOC_STRATEGY> vector_type;

stxxl::unsigned_type checksum(vector_type & input)
{
    stxxl::unsigned_type sum = 0;
    for (vector_type::const_iterator i = input.begin(); i != input.end(); ++i)
        sum += (*i)._key;
    return sum;
}

void linear_sort_normal(vector_type & input)
{
    stxxl::unsigned_type sum1 = checksum(input);

    stxxl::stats::get_instance()->reset();
    double start = stxxl::timestamp();

    stxxl::sort(input.begin(), input.end(), cmp_less_key(), run_size);

    double stop = stxxl::timestamp();
    std::cout << *(stxxl::stats::get_instance()) << std::endl;

    stxxl::unsigned_type sum2 = checksum(input);

    std::cout << sum1 << " ?= " << sum2 << std::endl;

    STXXL_MSG((stxxl::is_sorted<vector_type::const_iterator>(input.begin(), input.end()) ? "OK" : "NOT SORTED"));

    std::cout << "Linear sorting normal took " << (stop - start) << " seconds." << std::endl;
}

void linear_sort_streamed(vector_type & input, vector_type & output)
{
    stxxl::unsigned_type sum1 = checksum(input);

    stxxl::stats::get_instance()->reset();
    double start = stxxl::timestamp();

    typedef __typeof__(stxxl::stream::streamify(input.begin(), input.end())) input_stream_type;

    input_stream_type input_stream = stxxl::stream::streamify(input.begin(), input.end());


    typedef cmp_less_key comparator_type;
    comparator_type cl;

    typedef stxxl::stream::sort<input_stream_type, comparator_type, block_size> sort_stream_type;

    sort_stream_type sort_stream(input_stream, cl, run_size);

    vector_type::iterator o = stxxl::stream::materialize(sort_stream, output.begin(), output.end());

    double stop = stxxl::timestamp();
    std::cout << *(stxxl::stats::get_instance()) << std::endl;

    stxxl::unsigned_type sum2 = checksum(output);

    std::cout << sum1 << " ?= " << sum2 << std::endl;
    if (sum1 != sum2)
        STXXL_MSG("WRONG DATA");

    STXXL_MSG((stxxl::is_sorted<vector_type::const_iterator>(output.begin(), output.end(), comparator_type()) ? "OK" : "NOT SORTED"));

    std::cout << "Linear sorting streamed took " << (stop - start) << " seconds." << std::endl;
}


int main(int argc, const char ** argv)
{
    if (argc < 6) {
        std::cout << "Usage: " << argv[0] << " [n in megabytes] [p threads] [M in megabytes] [sorting algorithm: m | q | qb] [merging algorithm: p | s | n]" << std::endl;
        return -1;
    }

    stxxl::config::get_instance();

    unsigned long megabytes_to_process = atoi(argv[1]);
    int p = atoi(argv[2]);
    stxxl::unsigned_type memory_to_use = (stxxl::unsigned_type)atoi(argv[3]) * megabyte;
    run_size = memory_to_use;
    buffer_size = memory_to_use / 16;
#ifdef __MCSTL__
    mcstl::HEURISTIC::num_threads = p;
    mcstl::HEURISTIC::force_sequential = false;

    mcstl::HEURISTIC::merge_splitting = mcstl::HEURISTIC::EXACT;
    mcstl::HEURISTIC::merge_minimal_n = 10000;
    mcstl::HEURISTIC::merge_oversampling = 10;

    mcstl::HEURISTIC::multiway_merge_algorithm = mcstl::HEURISTIC::LOSER_TREE;
    mcstl::HEURISTIC::multiway_merge_splitting = mcstl::HEURISTIC::EXACT;
    mcstl::HEURISTIC::multiway_merge_oversampling = 10;
    mcstl::HEURISTIC::multiway_merge_minimal_n = 10000;
    mcstl::HEURISTIC::multiway_merge_minimal_k = 2;
    if (!strcmp(argv[4], "q"))                                       //quicksort
        mcstl::HEURISTIC::sort_algorithm = mcstl::HEURISTIC::QS;
    else if (!strcmp(argv[4], "qb"))                                 //balanced quicksort
        mcstl::HEURISTIC::sort_algorithm = mcstl::HEURISTIC::QS_BALANCED;
    else if (!strcmp(argv[4], "m"))                                  //merge sort
        mcstl::HEURISTIC::sort_algorithm = mcstl::HEURISTIC::MWMS;
    else /*if(!strcmp(argv[4], "s"))*/                               //sequential (default)
    {
        mcstl::HEURISTIC::sort_algorithm = mcstl::HEURISTIC::QS;
        mcstl::HEURISTIC::sort_minimal_n = memory_to_use;
    }

    if (!strcmp(argv[5], "p"))                                       //parallel
    {
        stxxl::SETTINGS::native_merge = false;
        //mcstl::HEURISTIC::multiway_merge_minimal_n = 1024;	     //leave as default
    }
    else if (!strcmp(argv[5], "s"))                                  //sequential
    {
        stxxl::SETTINGS::native_merge = false;
        mcstl::HEURISTIC::multiway_merge_minimal_n = memory_to_use;  //too much to be called
    }
    else /*if(!strcmp(argv[5], "n"))*/                               //native (default)
        stxxl::SETTINGS::native_merge = true;

    mcstl::HEURISTIC::multiway_merge_minimal_k = 2;
#endif

    std::cout << "Sorting " << megabytes_to_process << " MB of data ("
              << (megabytes_to_process * megabyte / sizeof(my_type)) << " elements) using "
              << (memory_to_use / megabyte) << " MB of internal memory and "
              << p << " thread(s), block size "
              << block_size << ", element size " << sizeof(my_type) << std::endl;

    const stxxl::int64 n_records =
        stxxl::int64(megabytes_to_process) * stxxl::int64(megabyte) / sizeof(my_type);
    vector_type input(n_records);

    stxxl::stats::get_instance()->reset();
    double generate_start = stxxl::timestamp();

    stxxl::generate(input.begin(), input.end(), stxxl::random_number64(), memory_to_use / STXXL_DEFAULT_BLOCK_SIZE(my_type));

    double generate_stop = stxxl::timestamp();
    std::cout << *(stxxl::stats::get_instance()) << std::endl;

    std::cout << "Generating took " << (generate_stop - generate_start) << " seconds." << std::endl;

    STXXL_MSG(((stxxl::is_sorted<vector_type::const_iterator>(input.begin(), input.end())) ? "OK" : "NOT SORTED"));

    {
        vector_type output(n_records);

        linear_sort_streamed(input, output);
        linear_sort_normal(input);
    }

    return 0;
}
// vim: et:ts=4:sw=4
