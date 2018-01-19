/***************************************************************************
 *  tools/benchmarks/monotonic_pq.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cassert>
#include <iostream>
#include <limits>
#include <queue>

#include <tlx/logger.hpp>

#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL 1
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL 1
#define STXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER 1

#define TINY_PQ 0
#define MANUAL_PQ 0

#define SIDE_PQ 1       // compare with second, in-memory PQ (needs a lot of memory)

#include <stxxl/priority_queue>
#include <stxxl/timer>

const size_t mega = 1024 * 1024;
constexpr bool debug = false;

#define RECORD_SIZE 16
#define LOAD 0

using my_key_type = uint64_t;

#define MAGIC 123

struct my_type
{
    using key_type = my_key_type;

    key_type key;
#if LOAD
    key_type load;
    char data[RECORD_SIZE - 2 * sizeof(key_type)];
#else
    char data[RECORD_SIZE - sizeof(key_type)];
#endif

    my_type() { }
    explicit my_type(key_type k) : key(k) { }
#if LOAD
    my_type(key_type k, key_type l) : key(k), load(l) { }
#endif

    void operator = (const key_type& k) { key = k; }
#if LOAD
    void operator = (const my_type& mt)
    {
        key = mt.key;
        load = mt.load;
    }
    bool operator == (const my_type& mt) { return (key == mt.key) && (load = mt.load); }
#else
    void operator = (const my_type& mt) { key = mt.key; }
    bool operator == (const my_type& mt) { return key == mt.key; }
#endif
};

std::ostream& operator << (std::ostream& o, const my_type& obj)
{
    o << obj.key;
#if LOAD
    o << "/" << obj.load;
#endif
    return o;
}

//STXXL priority queue is a _maximum_ PQ. "Greater" comparator makes this a "minimum" PQ again.

struct my_cmp /*: public std::binary_function<my_type, my_type, bool>*/ // greater
{
    using first_argument_type = my_type;
    using second_argument_type = my_type;
    using result_type = bool;

    bool operator () (const my_type& a, const my_type& b) const
    {
        return a.key > b.key;
    }

    my_type min_value() const
    {
#if LOAD
        return my_type(std::numeric_limits<my_type::key_type>::max(), MAGIC);
#else
        return my_type(std::numeric_limits<my_type::key_type>::max());
#endif
    }
    my_type max_value() const
    {
#if LOAD
        return my_type(std::numeric_limits<my_type::key_type>::min(), MAGIC);
#else
        return my_type(std::numeric_limits<my_type::key_type>::min());
#endif
    }
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " [n in MiB]"
            #if defined(STXXL_PARALLEL)
            << " [p threads]"
            #endif
            << std::endl;
        return -1;
    }

    LOG1 << "----------------------------------------";

    foxxll::config::get_instance();
    std::string Flags = std::string("")
#if STXXL_CHECK_ORDER_IN_SORTS
                        + " STXXL_CHECK_ORDER_IN_SORTS"
#endif
#ifdef NDEBUG
                        + " NDEBUG"
#endif
#if TINY_PQ
                        + " TINY_PQ"
#endif
#if MANUAL_PQ
                        + " MANUAL_PQ"
#endif
#if SIDE_PQ
                        + " SIDE_PQ"
#endif
#if STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
                        + " STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL"
#endif
#if STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
                        + " STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL"
#endif
#if STXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER
                        + " STXXL_PARALLEL_PQ_MULTIWAY_MERGE_DELETE_BUFFER"
#endif
    ; // NOLINT
    LOG1 << "Flags:" << Flags;

    unsigned long megabytes = atoi(argv[1]);
#if defined(STXXL_PARALLEL_MODE)
    int num_threads = atoi(argv[2]);
    LOG1 << "Threads: " << num_threads;

    omp_set_num_threads(num_threads);
    __gnu_parallel::_Settings parallel_settings(__gnu_parallel::_Settings::get());
    parallel_settings.sort_algorithm = __gnu_parallel::QS_BALANCED;
    parallel_settings.sort_splitting = __gnu_parallel::SAMPLING;
    parallel_settings.sort_minimal_n = 1000;
    parallel_settings.sort_mwms_oversampling = 10;

    parallel_settings.merge_splitting = __gnu_parallel::SAMPLING;
    parallel_settings.merge_minimal_n = 1000;
    parallel_settings.merge_oversampling = 10;

    parallel_settings.multiway_merge_algorithm = __gnu_parallel::LOSER_TREE;
    parallel_settings.multiway_merge_splitting = __gnu_parallel::EXACT;
    parallel_settings.multiway_merge_oversampling = 10;
    parallel_settings.multiway_merge_minimal_n = 1000;
    parallel_settings.multiway_merge_minimal_k = 2;
    __gnu_parallel::_Settings::set(parallel_settings);
#endif

    const size_t mem_for_queue = 512 * mega;
    const size_t mem_for_pools = 512 * mega;

#if TINY_PQ
    tlx::unused(mem_for_queue);
    const unsigned BufferSize1 = 32;               // equalize procedure call overheads etc.
    const unsigned N = (1 << 9) / sizeof(my_type); // minimal sequence length
    const unsigned IntKMAX = 8;                    // maximal arity for internal mergersq
    const unsigned IntLevels = 2;                  // number of internal levels
    const unsigned BlockSize = (4 * mega);
    const unsigned ExtKMAX = 8;                    // maximal arity for external mergers
    const unsigned ExtLevels = 2;                  // number of external levels
    using pq_type = stxxl::priority_queue<
              stxxl::priority_queue_config<
                  my_type,
                  my_cmp,
                  BufferSize1,
                  N,
                  IntKMAX,
                  IntLevels,
                  BlockSize,
                  ExtKMAX,
                  ExtLevels
                  >
              >;
#elif MANUAL_PQ
    tlx::unused(mem_for_queue);
    const unsigned BufferSize1 = 32;                    // equalize procedure call overheads etc.
    const unsigned N = (1 << 20) / sizeof(my_type);     // minimal sequence length
    const unsigned IntKMAX = 16;                        // maximal arity for internal mergersq
    const unsigned IntLevels = 2;                       // number of internal levels
    const unsigned BlockSize = (4 * mega);
    const unsigned ExtKMAX = 32;                        // maximal arity for external mergers
    const unsigned ExtLevels = 2;                       // number of external levels
    using pq_type = stxxl::priority_queue<
              stxxl::priority_queue_config<
                  my_type,
                  my_cmp,
                  BufferSize1,
                  N,
                  IntKMAX,
                  IntLevels,
                  BlockSize,
                  ExtKMAX,
                  ExtLevels
                  >
              >;
#else
    const uint64_t volume = uint64_t(200000) * mega;     // in bytes
    using gen = stxxl::PRIORITY_QUEUE_GENERATOR<my_type, my_cmp, mem_for_queue, volume / sizeof(my_type) / 1024 + 1>;
    using pq_type = gen::result;
#endif

    LOG1 << "Internal arity: " << pq_type::IntKMAX;
    LOG1 << "N : " << pq_type::N; //X / (AI * AI)
    LOG1 << "External arity: " << pq_type::ExtKMAX;
    LOG1 << "Block size B: " << pq_type::BlockSize;
    //EConsumption = X * settings::E + settings::B * AE + ((MaxS_ / X) / AE) * settings::B * 1024

    LOG1 << "Data type size: " << sizeof(my_type);
    LOG1 << "";

    foxxll::stats_data sd_start(*foxxll::stats::get_instance());
    foxxll::timer Timer;
    Timer.start();

    pq_type p(mem_for_pools / 2, mem_for_pools / 2);
    uint64_t nelements = uint64_t(megabytes) * mega / sizeof(my_type);

    LOG1 << "Internal memory consumption of the priority queue: " << p.mem_cons() << " B";
    LOG1 << "Peak number of elements (n): " << nelements;
    LOG1 << "Max number of elements to contain: " << (uint64_t(pq_type::N) * pq_type::IntKMAX * pq_type::IntKMAX * pq_type::ExtKMAX * pq_type::ExtKMAX);
    srand(5);
    my_cmp cmp;
    my_key_type r, sum_input = 0, sum_output = 0;
    my_type least(0), last_least(0);

    const my_key_type modulo = 0x10000000;

#if SIDE_PQ
    std::priority_queue<my_type, std::vector<my_type>, my_cmp> side_pq;
#endif

    my_type side_pq_least;

    LOG1 << "op-sequence(monotonic pq): ( push, pop, push ) * n";
    for (uint64_t i = 0; i < nelements; ++i)
    {
        if ((i % mega) == 0)
            LOG1 << std::fixed << std::setprecision(2) << std::setw(5)
                 << (100.0 * (double)i / (double)nelements) << "% "
                 << "Inserting element " << i << " top() == " << least.key << " @ "
                 << std::setprecision(3) << Timer.seconds() << " s"
                 << std::setprecision(6) << std::resetiosflags(std::ios_base::floatfield);

        //monotone priority queue
        r = least.key + rand() % modulo;
        sum_input += r;
        p.push(my_type(r));
#if SIDE_PQ
        side_pq.push(my_type(r));
#endif

        least = p.top();
        sum_output += least.key;
        p.pop();
#if SIDE_PQ
        side_pq_least = side_pq.top();
        side_pq.pop();
        if (!(side_pq_least == least))
            LOG1 << "Wrong result at  " << i << "  " << side_pq_least.key << " != " << least.key;
#endif

        if (cmp(last_least, least))
        {
            LOG1 << "Wrong order at  " << i << "  " << last_least.key << " > " << least.key;
        }
        else
            last_least = least;

        r = least.key + rand() % modulo;
        sum_input += r;
        p.push(my_type(r));
#if SIDE_PQ
        side_pq.push(my_type(r));
#endif
    }
    Timer.stop();
    LOG1 << "Time spent for filling: " << Timer.seconds() << " s";

    LOG1 << "Internal memory consumption of the priority queue: " << p.mem_cons() << " B";
    foxxll::stats_data sd_middle(*foxxll::stats::get_instance());
    std::cout << sd_middle - sd_start;
    Timer.reset();
    Timer.start();

    LOG1 << "op-sequence(monotonic pq): ( pop, push, pop ) * n";
    for (uint64_t i = 0; i < nelements; ++i)
    {
        assert(!p.empty());

        least = p.top();
        sum_output += least.key;
        p.pop();
#if SIDE_PQ
        side_pq_least = side_pq.top();
        side_pq.pop();
        if (!(side_pq_least == least))
        {
            LOG << "" << side_pq_least << " != " << least;
        }
#endif
        if (cmp(last_least, least))
        {
            LOG1 << "Wrong result at " << i << "  " << last_least.key << " > " << least.key;
        }
        else
            last_least = least;

        r = least.key + rand() % modulo;
        sum_input += r;
        p.push(my_type(r));
#if SIDE_PQ
        side_pq.push(my_type(r));
#endif

        least = p.top();
        sum_output += least.key;
        p.pop();
#if SIDE_PQ
        side_pq_least = side_pq.top();
        side_pq.pop();
        if (!(side_pq_least == least))
        {
            LOG << "" << side_pq_least << " != " << least;
        }
#endif
        if (cmp(last_least, least))
        {
            LOG1 << "Wrong result at " << i << "  " << last_least.key << " > " << least.key;
        }
        else
            last_least = least;

        if ((i % mega) == 0)
            LOG1 << std::fixed << std::setprecision(2) << std::setw(5)
                 << (100.0 * (double)i / (double)nelements) << "% "
                 << "Popped element " << i << " == " << least.key << " @ "
                 << std::setprecision(3) << Timer.seconds() << " s"
                 << std::setprecision(6) << std::resetiosflags(std::ios_base::floatfield);
    }
    LOG1 << "Last element " << nelements << " popped";
    Timer.stop();

    if (sum_input != sum_output)
        LOG1 << "WRONG sum! " << sum_input << " - " << sum_output << " = " << (sum_output - sum_input) << " / " << (sum_input - sum_output);

    LOG1 << "Time spent for removing elements: " << Timer.seconds() << " s";
    LOG1 << "Internal memory consumption of the priority queue: " << p.mem_cons() << " B";
    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - sd_middle;
    std::cout << *foxxll::stats::get_instance();

    assert(sum_input == sum_output);
}
