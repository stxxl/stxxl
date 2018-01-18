/***************************************************************************
 *  tools/benchmark_pqueue.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

static const char* description =
    "Benchmark the priority queue implementation using a sequence of "
    "operations. The PQ contains pairs of 32- or 64-bit integers, or a "
    "24 byte struct. The operation sequence is either a simple fill/delete "
    "cycle or fill/intermixed inserts/deletes. Because the memory parameters "
    "of the PQ must be set a compile-time, the benchmark provides only "
    "three PQ sizes: for 256 MiB, 1 GiB and 8 GiB of RAM, with the maximum "
    "number of items set accordingly.";

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/common/padding.h>
#include <stxxl/bits/common/tuple.h>
#include <stxxl/priority_queue>
#include <stxxl/random>
#include <stxxl/timer>

using stxxl::external_size_type;

#define MiB (1024 * 1024)
#define PRINTMOD (16 * MiB)

// *** Integer Pair Types

using uint32_pair_type = stxxl::tuple<uint32_t, uint32_t>;

using uint64_pair_type = stxxl::tuple<uint64_t, uint64_t>;

// *** Larger Structure Type

#define MY_TYPE_SIZE 24

struct my_type
    : stxxl::padding<MY_TYPE_SIZE - sizeof(uint32_pair_type)>,
      public uint32_pair_type
{
    using key_type = uint32_t;

    my_type() { }
    my_type(const key_type& k1, const key_type& k2)
        : uint32_pair_type(k1, k2)
    { }

    static my_type max_value()
    {
        return my_type(std::numeric_limits<key_type>::max(),
                       std::numeric_limits<key_type>::max());
    }
};

template <typename ValueType>
struct my_cmp : public std::binary_function<ValueType, ValueType, bool>
{
    bool operator () (const ValueType& a, const ValueType& b) const
    {
        // PQ is a max priority queue, thus compare greater
        return a.first > b.first;
    }

    ValueType min_value() const
    {
        return ValueType::max_value();
    }
};

static inline void progress(const char* text, external_size_type i, external_size_type nelements)
{
    if ((i % PRINTMOD) == 0)
        LOG1 << text << " " << i << " ("
             << std::setprecision(5)
             << ((double)i * 100.0 / (double)nelements) << " %)";
}

template <typename PQType>
void run_pqueue_insert_delete(external_size_type nelements, size_t mem_for_pools)
{
    using ValueType = typename PQType::value_type;
    using KeyType = typename ValueType::first_type;

    // construct priority queue
    PQType pq(mem_for_pools / 2, mem_for_pools / 2);

    pq.dump_sizes();

    LOG1 << "Internal memory consumption of the priority queue: " << pq.mem_cons() << " B";
    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    {
        foxxll::scoped_print_timer timer("Filling PQ", nelements * sizeof(ValueType));

        for (external_size_type i = 0; i < nelements; i++)
        {
            progress("Inserting element", i, nelements);

            pq.push(ValueType(static_cast<KeyType>(nelements - i), 0));
        }
    }

    die_unless(pq.size() == nelements);

    LOG1 << "Internal memory consumption of the priority queue: " << pq.mem_cons() << " B";

    pq.dump_sizes();

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
    stats_begin = *foxxll::stats::get_instance();

    {
        foxxll::scoped_print_timer timer("Reading PQ", nelements * sizeof(ValueType));

        for (external_size_type i = 0; i < nelements; ++i)
        {
            die_unless(!pq.empty());
            die_unless(pq.top().first == i + 1);

            pq.pop();

            progress("Popped element", i, nelements);
        }
    }

    LOG1 << "Internal memory consumption of the priority queue: " << pq.mem_cons() << " B";
    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
}

template <typename PQType>
void run_pqueue_insert_intermixed(external_size_type nelements, size_t mem_for_pools)
{
    using ValueType = typename PQType::value_type;
    using KeyType = typename ValueType::first_type;

    // construct priority queue
    PQType pq(mem_for_pools / 2, mem_for_pools / 2);

    pq.dump_sizes();

    LOG1 << "Internal memory consumption of the priority queue: " << pq.mem_cons() << " B";
    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    {
        foxxll::scoped_print_timer timer("Filling PQ", nelements * sizeof(ValueType));

        for (external_size_type i = 0; i < nelements; i++)
        {
            progress("Inserting element", i, nelements);

            pq.push(ValueType(static_cast<KeyType>(nelements - i), 0));
        }
    }

    die_unless(pq.size() == nelements);

    LOG1 << "Internal memory consumption of the priority queue: " << pq.mem_cons() << " B";

    pq.dump_sizes();

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
    stats_begin = *foxxll::stats::get_instance();

    stxxl::random_number32 rand;

    {
        foxxll::scoped_print_timer timer("Intermixed Insert/Delete", nelements * sizeof(ValueType));

        for (external_size_type i = 0; i < nelements; ++i)
        {
            int o = rand() % 3;
            if (o == 0)
            {
                pq.push(ValueType(static_cast<KeyType>(nelements - i), 0));
            }
            else
            {
                die_unless(!pq.empty());
                pq.pop();
            }

            progress("Intermixed element", i, nelements);
        }
    }

    LOG1 << "Internal memory consumption of the priority queue: " << pq.mem_cons() << " B";

    pq.dump_sizes();

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
}

template <typename ValueType,
          size_t mib_for_queue, size_t mib_for_pools,
          external_size_type maxvolume>
int do_benchmark_pqueue(external_size_type volume, unsigned opseq)
{
    const size_t mem_for_queue = mib_for_queue * MiB;
    const size_t mem_for_pools = mib_for_pools * MiB;
    using gen = typename stxxl::PRIORITY_QUEUE_GENERATOR<
              ValueType, my_cmp<ValueType>,
              mem_for_queue,
              maxvolume* MiB / sizeof(ValueType)>;

    using pq_type = typename gen::result;

    LOG1 << "Given PQ parameters: " << mib_for_queue << " MiB for queue, "
         << mib_for_pools << " MiB for pools, " << maxvolume << " GiB maximum volume.";

    LOG1 << "Selected PQ parameters:";
    LOG1 << "element size: " << sizeof(ValueType);
    LOG1 << "block size: " << pq_type::BlockSize;
    LOG1 << "insertion buffer size (N): " << pq_type::N << " items ("
         << pq_type::N * sizeof(ValueType) << " B)";
    LOG1 << "delete buffer size: " << pq_type::kDeleteBufferSize;
    LOG1 << "maximal arity for internal mergers (AI): " << pq_type::IntKMAX;
    LOG1 << "maximal arity for external mergers (AE): " << pq_type::ExtKMAX;
    LOG1 << "internal groups: " << pq_type::kNumIntGroups;
    LOG1 << "external groups: " << pq_type::kNumExtGroups;
    LOG1 << "X : " << gen::X;

    if (volume == 0) volume = 2 * (mem_for_queue + mem_for_pools);

    external_size_type nelements = volume / sizeof(ValueType);

    LOG1 << "Number of elements: " << nelements;

    if (opseq == 0)
    {
        run_pqueue_insert_delete<pq_type>(nelements, mem_for_pools);
        run_pqueue_insert_intermixed<pq_type>(nelements, mem_for_pools);
    }
    else if (opseq == 1)
        run_pqueue_insert_delete<pq_type>(nelements, mem_for_pools);
    else if (opseq == 2)
        run_pqueue_insert_intermixed<pq_type>(nelements, mem_for_pools);
    else
        LOG1 << "Invalid operation sequence.";

    return 1;
}

template <typename ValueType>
int do_benchmark_pqueue_config(unsigned pqconfig, external_size_type size, unsigned opseq)
{
    if (pqconfig == 0)
    {
        do_benchmark_pqueue_config<ValueType>(1, size, opseq);
        do_benchmark_pqueue_config<ValueType>(2, size, opseq);
        do_benchmark_pqueue_config<ValueType>(3, size, opseq);
        return 1;
    }
    else if (pqconfig == 1)
        return do_benchmark_pqueue<ValueType, 128, 128, 16>(size, opseq);
    else if (pqconfig == 2)
        return do_benchmark_pqueue<ValueType, 512, 512, 64>(size, opseq);
#if __x86_64__ || __LP64__ || (__WORDSIZE == 64)
    else if (pqconfig == 3)
        return do_benchmark_pqueue<ValueType, 4096, 4096, 512>(size, opseq);
#endif
    else
        return 0;
}

int do_benchmark_pqueue_type(unsigned type, unsigned pqconfig, external_size_type size, unsigned opseq)
{
    if (type == 0)
    {
        do_benchmark_pqueue_type(1, pqconfig, size, opseq);
        do_benchmark_pqueue_type(2, pqconfig, size, opseq);
        do_benchmark_pqueue_type(3, pqconfig, size, opseq);
        return 1;
    }
    else if (type == 1)
        return do_benchmark_pqueue_config<uint32_pair_type>(pqconfig, size, opseq);
    else if (type == 2)
        return do_benchmark_pqueue_config<uint64_pair_type>(pqconfig, size, opseq);
    else if (type == 3)
        return do_benchmark_pqueue_config<my_type>(pqconfig, size, opseq);
    else
        return 0;
}

int benchmark_pqueue(int argc, char* argv[])
{
    // parse command line
    stxxl::cmdline_parser cp;

    cp.set_description(description);

    external_size_type size = 0;
    cp.add_opt_param_bytes("size", size,
                           "Amount of data to insert (e.g. 1GiB)");

    unsigned type = 2;
    cp.add_uint('t', "type", type,
                "Value type of tested priority queue:\n"
                " 1 = pair of uint32_t,\n"
                " 2 = pair of uint64_t (default),\n"
                " 3 = 24 byte struct\n"
                " 0 = all of the above");

    unsigned pqconfig = 2;
    cp.add_uint('p', "pq", pqconfig,
                "Priority queue configuration to test:\n"
                "1 = small (256 MiB RAM, 4 GiB elements)\n"
                "2 = medium (1 GiB RAM, 16 GiB elements) (default)\n"
#if __x86_64__ || __LP64__ || (__WORDSIZE == 64)
                "3 = big (8 GiB RAM, 64 GiB elements)\n"
#endif
                "0 = all of the above");

    unsigned opseq = 1;
    cp.add_uint('o', "opseq", opseq,
                "Operation sequence to perform:\n"
                " 1 = insert all, delete all (default)\n"
                " 2 = insert all, intermixed insert/delete\n"
                " 0 = all of the above");

    if (!cp.process(argc, argv))
        return -1;

    foxxll::config::get_instance();

    if (!do_benchmark_pqueue_type(type, pqconfig, size, opseq))
    {
        LOG1 << "Invalid (type,pqconfig) combination.";
    }

    return 0;
}
