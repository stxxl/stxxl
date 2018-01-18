/***************************************************************************
 *  tests/containers/test_matrix.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <limits>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/die_with_message.hpp>

#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/containers/matrix.h>
#include <stxxl/stream>
#include <stxxl/vector>

// forced instantiation
template class stxxl::matrix<int, 32>;
template class stxxl::matrix_iterator<int, 32>;
template class stxxl::const_matrix_iterator<int, 32>;
template class stxxl::matrix_row_major_iterator<int, 32>;
template class stxxl::matrix_col_major_iterator<int, 32>;
template class stxxl::const_matrix_row_major_iterator<int, 32>;
template class stxxl::const_matrix_col_major_iterator<int, 32>;
template class stxxl::column_vector<int>;
template class stxxl::row_vector<int>;
template struct stxxl::matrix_local::matrix_operations<int, 32>;
/*

struct constant_one
{
    const constant_one& operator ++ () const { return *this; }
    bool empty() const { return false; }
    int operator * () const { return 1; }
};

struct modulus_integers
{
private:
    size_t step, counter, modulus;

public:
    modulus_integers(size_t start = 1, size_t step = 1, size_t modulus = 0)
        : step(step),
          counter(start),
          modulus(modulus)
    { }

    modulus_integers& operator ++ ()
    {
        counter += step;
        if (modulus != 0 && counter >= modulus)
            counter %= modulus;
        return *this;
    }

    bool empty() const { return false; }

    size_t operator * () const { return counter; }
};

struct diagonal_matrix
{
private:
    size_t order, counter, value;

public:
    diagonal_matrix(size_t order, size_t value = 1)
        : order(order), counter(0), value(value) { }

    diagonal_matrix& operator ++ ()
    {
        ++counter;
        return *this;
    }

    bool empty() const { return false; }

    size_t operator * () const
    { return (counter % (order + 1) == 0) * value; }
};

struct inverse_diagonal_matrix
{
private:
    size_t order, counter, value;

public:
    inverse_diagonal_matrix(size_t order, size_t value = 1)
        : order(order), counter(0), value(value) { }

    inverse_diagonal_matrix& operator ++ ()
    {
        ++counter;
        return *this;
    }

    bool empty() const { return false; }

    size_t operator * () const
    { return (counter % order == order - 1 - counter / order) * value; }
};

template <class CompareIterator, typename ValueType>
class iterator_compare
{
    using error_type =  std::pair<ValueType, ValueType> ;

    CompareIterator& compiter;
    ValueType current_value;
    stxxl::vector<error_type> errors;

public:
    iterator_compare(CompareIterator& co)
        : compiter(co),
          current_value(),
          errors()
    { }

    iterator_compare& operator ++ ()
    {
        if (current_value != *compiter)
            errors.push_back(error_type(current_value, *compiter));
        ++compiter;
        return *this;
    }

    bool empty() const { return compiter.empty(); }
    ValueType& operator * () { return current_value; }

    size_t get_num_errors() { return errors.size(); }
    stxxl::vector<error_type> & get_errors() { return errors; }
};
*/

const int small_block_order = 32; // must be a multiple of 32, assuming at least 4 bytes element size
const int block_order = 32;       // must be a multiple of 32, assuming at least 4 bytes element size

uint64_t internal_memory = 16 * 1024 * 1024;

void test1(int rank)
{
    LOG1 << "multiplying two int64_t matrices of rank " << rank << " block order " << small_block_order;
    using value_type = int64_t;

    using block_scheduler_type = foxxll::block_scheduler<
              stxxl::matrix_swappable_block<value_type, small_block_order> >;
    using matrix_type = stxxl::matrix<value_type, small_block_order>;
    using row_vector_type = matrix_type::row_vector_type;
    using column_vector_type = matrix_type::column_vector_type;
    using row_major_iterator = matrix_type::row_major_iterator;
    using const_row_major_iterator = matrix_type::const_row_major_iterator;

    // the block_scheduler may use internal_memory byte for caching
    block_scheduler_type* bs_ptr = new block_scheduler_type(internal_memory);
    // the block_scheduler may use 16 blocks for caching
    //block_scheduler_type * bs_ptr = new block_scheduler_type(16 * sizeof(value_type) * small_block_order * small_block_order);
    block_scheduler_type& bs = *bs_ptr;

    // create three matrices
    matrix_type
    * a = new matrix_type(bs, rank, rank),
        * b = new matrix_type(bs, rank, rank),
        * c = new matrix_type(bs, rank, rank);

    foxxll::stats_data stats_before, stats_after;
    stxxl::matrix_operation_statistic_data matrix_stats_before, matrix_stats_after;

    // ------ first run
    for (row_major_iterator mit = a->begin(); mit != a->end(); ++mit)
        *mit = 1;
    for (row_major_iterator mit = b->begin(); mit != b->end(); ++mit)
        *mit = 1;

    bs.flush();
    LOG1 << "start mult";
    matrix_stats_before.set();
    stats_before = *foxxll::stats::get_instance();

    *c = (*a) * (*b);
    bs.flush();

    stats_after = *foxxll::stats::get_instance();
    matrix_stats_after.set();
    LOG1 << "end mult";

    LOG1 << matrix_stats_after - matrix_stats_before;
    LOG1 << stats_after - stats_before;
    {
        size_t num_err = 0;
        for (const_row_major_iterator mit = c->cbegin(); mit != c->cend(); ++mit)
            num_err += (*mit != rank);
        die_with_message_unless(num_err == 0,
                                "c had " << num_err << " errors");
    }

    // ------ second run
    {
        size_t i = 1;
        for (row_major_iterator mit = a->begin(); mit != a->end(); ++mit, ++i)
            *mit = i;
    }
    {
        b->set_zero();
        matrix_type::iterator mit = b->begin();
        for (size_t i = 0; i < b->get_height(); ++i)
        {
            mit.set_pos(i, i);
            *mit = 1;
        }
    }

    bs.flush();
    LOG1 << "start mult";
    matrix_stats_before.set();
    stats_before = *foxxll::stats::get_instance();

    *c = (*a) * (*b);
    bs.flush();

    stats_after = *foxxll::stats::get_instance();
    matrix_stats_after.set();
    LOG1 << "end mult";

    *c *= 3;
    *c += *a;

    LOG1 << matrix_stats_after - matrix_stats_before;
    LOG1 << stats_after - stats_before;
    {
        size_t num_err = 0;
        int64_t i = 1;
        for (const_row_major_iterator mit = c->cbegin(); mit != c->cend(); ++mit, ++i)
            num_err += (*mit != (i * 4));

        die_with_message_unless(num_err == 0,
                                "c had " << num_err << " errors");
    }

    {
        column_vector_type x(rank), y;
        size_t i = 0;
        for (column_vector_type::iterator it = x.begin(); it != x.end(); ++it)
            *it = ++i;
        y = *b * x;
        y = y + x;
        y += x;
        y = y - x;
        y -= x;
        y = x * 5;
        y *= 5;

        row_vector_type w(rank), z;
        i = 0;
        for (row_vector_type::iterator it = w.begin(); it != w.end(); ++it)
            *it = ++i;
        z = w * (*b);
        z = z + w;
        z += w;
        z = z - w;
        z -= w;
        z = w * 5;
        z *= 5;

        *a = matrix_type(bs, x, w);

        value_type v;
        v = w * x;

        tlx::unused(v);
    }

    delete a;
    delete b;
    delete c;
    delete bs_ptr;
}

void test2(int rank, int mult_algo_num, int sched_algo_num)
{
    LOG1 << "multiplying two full double matrices of rank " << rank << ", block order " << block_order
         << " using " << internal_memory << " bytes internal memory, multiplication-algo "
         << mult_algo_num << ", scheduling-algo " << sched_algo_num;

    using value_type = double;

    using block_scheduler_type = foxxll::block_scheduler<
              stxxl::matrix_swappable_block<value_type, block_order> >;
    using matrix_type = stxxl::matrix<value_type, block_order>;
    using row_major_iterator = matrix_type::row_major_iterator;
    using const_row_major_iterator = matrix_type::const_row_major_iterator;

    // the block_scheduler may use internal_memory byte for caching
    block_scheduler_type* bs_ptr = new block_scheduler_type(internal_memory);
    block_scheduler_type& bs = *bs_ptr;
    matrix_type
    * a = new matrix_type(bs, rank, rank),
        * b = new matrix_type(bs, rank, rank),
        * c = new matrix_type(bs, rank, rank);
    foxxll::stats_data stats_before, stats_after;
    stxxl::matrix_operation_statistic_data matrix_stats_before, matrix_stats_after;

    LOG1 << "writing input matrices";
    for (row_major_iterator mit = a->begin(); mit != a->end(); ++mit)
        *mit = 1;
    for (row_major_iterator mit = b->begin(); mit != b->end(); ++mit)
        *mit = 1;

    bs.flush();
    LOG1 << "start of multiplication";
    matrix_stats_before.set();
    stats_before = *foxxll::stats::get_instance();

    if (mult_algo_num >= 0)
        *c = a->multiply(*b, mult_algo_num, sched_algo_num);
    else
        *c = a->multiply_internal(*b, sched_algo_num);

    bs.flush();
    stats_after = *foxxll::stats::get_instance();
    matrix_stats_after.set();
    LOG1 << "end of multiplication";

    LOG1 << matrix_stats_after - matrix_stats_before;
    LOG1 << stats_after - stats_before;
    {
        size_t num_err = 0;
        for (const_row_major_iterator mit = c->cbegin(); mit != c->cend(); ++mit)
            num_err += (*mit != rank);
        die_with_message_unless(num_err == 0, "c had " << num_err << " errors");
    }

    delete a;
    delete b;
    delete c;
    delete bs_ptr;
}

int main(int argc, char** argv)
{
    int test_case = -1;
    int rank = 200;
    int mult_algo_num = 2;
    int sched_algo_num = 1;

    stxxl::cmdline_parser cp;

    cp.set_description("stxxl matrix test");
    cp.set_author("Raoul Steffen <R-Steffen@gmx.de>");

    cp.add_opt_param_int(
        "K", test_case,
        "number of the test case to run: 1 or 2, or by default: all");

    cp.add_int('r', "rank", "<N>", rank,
               "rank of the matrices, default: 500");

    cp.add_bytes('m', "memory", "<L>", internal_memory,
                 "internal memory to, default: 256 MiB");

    cp.add_int('a', "mult-algo", "<N>", mult_algo_num,
               "use multiplication-algorithm number N\n"
               "  available are:\n"
               "   0: naive_multiply_and_add\n"
               "   1: recursive_multiply_and_add\n"
               "   2: strassen_winograd_multiply_and_add\n"
               "   3: multi_level_strassen_winograd_multiply_and_add\n"
               "   4: strassen_winograd_multiply (block-interleaved pre- and postadditions)\n"
               "   5: strassen_winograd_multiply_and_add_interleaved (block-interleaved preadditions)\n"
               "   6: multi_level_strassen_winograd_multiply_and_add_block_grained\n"
               "  default: 2");

    cp.add_int('s', "scheduling-algo", "<N>", sched_algo_num,
               "use scheduling-algorithm number N\n"
               "  available are:\n"
               "   0: online LRU\n"
               "   1: offline LFD\n"
               "   2: offline LRU prefetching\n"
               "  default: 1");

    if (!cp.process(argc, argv))
        return 0;

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    switch (test_case)
    {
    default:
        test1(rank);

        for (int mult_algo = 0; mult_algo <= 6; ++mult_algo)
        {
            for (int sched_algo = 0; sched_algo <= 2; ++sched_algo)
            {
                test2(rank, mult_algo, sched_algo);
            }
        }
        break;
    case 1:
        test1(rank);
        break;
    case 2:
        test2(rank, mult_algo_num, sched_algo_num);
        break;
    }

    LOG1 << "end of test";

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    return 0;
}
