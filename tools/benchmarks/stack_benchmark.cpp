/***************************************************************************
 *  tools/benchmarks/stack_benchmark.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/stack_benchmark.cpp
//! This is a benchmark mentioned in the paper
//! R. Dementiev, L. Kettner, P. Sanders "STXXL: standard template library for XXL data sets"
//! Software: Practice and Experience
//! Volume 38, Issue 6, Pages 589-637, May 2008
//! DOI: 10.1002/spe.844

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/stack>
#include <stxxl/timer>

#define MEM_2_RESERVE    (768 * 1024 * 1024)

#ifndef BLOCK_SIZE
 #define    BLOCK_SIZE (2 * 1024 * 1024)
#endif

#ifndef DISKS
 #define DISKS 4
#endif

template <unsigned RECORD_SIZE>
struct my_record_
{
    char data[RECORD_SIZE];
    my_record_()
    {
        memset(data, 0, sizeof(data));
    }
};

template <unsigned RECORD_SIZE>
inline std::ostream& operator << (std::ostream& o, const my_record_<RECORD_SIZE>&)
{
    o << ".";
    return o;
}

template <typename stack_type>
void benchmark_insert(stack_type& Stack, uint64_t volume)
{
    using value_type = typename stack_type::value_type;

    LOG1 << "Record size: " << sizeof(value_type) << " bytes";

    uint64_t ops = volume / sizeof(value_type);

    value_type cur;

    // test whether top() returns an lvalue
    Stack.push(cur);
    Stack.top() = cur;
    Stack.pop();

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    foxxll::timer Timer;
    Timer.start();

    for (uint64_t i = 0; i < ops; ++i)
    {
        Stack.push(cur);
    }

    Timer.stop();

    LOG1 << "Records in Stack: " << Stack.size();
    die_unequal(ops, uint64_t(Stack.size()));

    LOG1 << "Insertions elapsed time: " << (Timer.mseconds() / 1000.) <<
        " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
        " MiB/s";

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
}

template <typename stack_type>
void benchmark_delete(stack_type& Stack, uint64_t volume)
{
    using value_type = typename stack_type::value_type;

    uint64_t ops = volume / sizeof(value_type);

    value_type cur;

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    foxxll::timer Timer;
    Timer.start();

    for (uint64_t i = 0; i < ops; ++i)
    {
        Stack.pop();
    }

    Timer.stop();

    die_unless(Stack.empty());

    LOG1 << "Deletions elapsed time: " << (Timer.mseconds() / 1000.) <<
        " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
        " MiB/s";

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
}

template <class my_record>
void run_stxxl_growshrink2_stack(uint64_t volume)
{
    using stack_type = typename stxxl::STACK_GENERATOR<my_record, stxxl::external,
                                                       stxxl::grow_shrink2, DISKS, BLOCK_SIZE>::result;
    using block_type = typename stack_type::block_type;

    foxxll::read_write_pool<block_type> pool(DISKS * 4, DISKS * 4);
    stack_type Stack(pool);

    benchmark_insert(Stack, volume);

    Stack.set_prefetch_aggr(DISKS * 8);

    benchmark_delete(Stack, volume);
}

template <class my_record>
void run_stxxl_normal_stack(uint64_t volume)
{
    using stack_type = typename stxxl::STACK_GENERATOR<my_record, stxxl::external,
                                                       stxxl::normal, DISKS, BLOCK_SIZE>::result;

    stack_type Stack;

    benchmark_insert(Stack, volume);
    benchmark_delete(Stack, volume);
}

template <class my_record>
void run_stl_stack(uint64_t volume)
{
    using stack_type = std::stack<my_record>;

    stack_type Stack;

    benchmark_insert(Stack, volume);
    benchmark_delete(Stack, volume);
}

int main(int argc, char* argv[])
{
    LOG1 << "stxxl::pq block size: " << BLOCK_SIZE << " bytes";

#if STXXL_DIRECT_IO_OFF
    LOG1 << "STXXL_DIRECT_IO_OFF is defined";
#else
    LOG1 << "STXXL_DIRECT_IO_OFF is NOT defined";
#endif

    if (argc < 3)
    {
        LOG1 << "Usage: " << argv[0] << " variant volume";
        LOG1 << "\t variant = 1: grow-shrink-stack2 with 4 byte records";
        LOG1 << "\t variant = 2: grow-shrink-stack2 with 32 byte records";
        LOG1 << "\t variant = 3: normal-stack with 4 byte records";
        LOG1 << "\t variant = 4: normal-stack with 32 byte records";
        LOG1 << "\t variant = 5: std::stack with 4 byte records";
        LOG1 << "\t variant = 6: std::stack with 32 byte records";
        LOG1 << "\t volume:      in bytes";
        return -1;
    }

    int variant = atoi(argv[1]);
    uint64_t volume = foxxll::atouint64(argv[2]);

    LOG1 << "Allocating array with size " <<
        MEM_2_RESERVE << " bytes to prevent file buffering.";
    int* array = new int[MEM_2_RESERVE / sizeof(int)];
    std::fill(array, array + (MEM_2_RESERVE / sizeof(int)), 0);

    LOG1 << "Running variant: " << variant;
    LOG1 << "Data volume    : " << volume << " bytes";

    switch (variant)
    {
    case 1:
        run_stxxl_growshrink2_stack<my_record_<4> >(volume);
        break;
    case 2:
        run_stxxl_growshrink2_stack<my_record_<32> >(volume);
        break;
    case 3:
        run_stxxl_normal_stack<my_record_<4> >(volume);
        break;
    case 4:
        run_stxxl_normal_stack<my_record_<32> >(volume);
        break;
    case 5:
        run_stl_stack<my_record_<4> >(volume);
        break;
    case 6:
        run_stl_stack<my_record_<32> >(volume);
        break;
    default:
        LOG1 << "Unsupported variant " << variant;
    }

    delete[] array;
}
