/***************************************************************************
 *  containers/test_matrix.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <limits>

// Thanks Daniel Russel, Stanford University
#include <Argument_helper.h>

#include <stxxl/vector>
#include <stxxl/stream>
#include <stxxl/bits/containers/matrix.h>

using namespace stxxl;

struct constant_one
{
    const constant_one & operator ++ () const { return *this; }
    bool empty() const { return false; }
    int operator * () const { return 1; }
};

struct modulus_integers
{
private:
    unsigned_type step, counter, modulus;

public:
    modulus_integers(unsigned_type start = 1, unsigned_type step = 1, unsigned_type modulus = 0)
        : step(step),
          counter(start),
          modulus(modulus)
    { }

    modulus_integers & operator ++ ()
    {
        counter += step;
        if (modulus != 0 && counter >= modulus)
            counter %= modulus;
        return *this;
    }

    bool empty() const { return false; }

    unsigned_type operator * () const { return counter; }
};

struct diagonal_matrix
{
private:
    unsigned_type order, counter, value;

public:
    diagonal_matrix(unsigned_type order, unsigned_type value = 1)
        : order(order), counter(0), value(value) { }

    diagonal_matrix & operator ++ ()
    {
        ++counter;
        return *this;
    }

    bool empty() const { return false; }

    unsigned_type operator * () const
    { return (counter % (order + 1) == 0) * value; }
};

struct inverse_diagonal_matrix
{
private:
    unsigned_type order, counter, value;

public:
    inverse_diagonal_matrix(unsigned_type order, unsigned_type value = 1)
        : order(order), counter(0), value(value) { }

    inverse_diagonal_matrix & operator ++ ()
    {
        ++counter;
        return *this;
    }

    bool empty() const { return false; }

    unsigned_type operator * () const
    { return (counter % order == order - 1 - counter / order) * value; }
};

template <class CompareIterator, typename ValueType>
class iterator_compare
{
    typedef std::pair<ValueType, ValueType> error_type;

    CompareIterator & compiter;
    ValueType current_value;
    vector<error_type> errors;

public:
    iterator_compare(CompareIterator & co)
        : compiter(co),
          current_value(),
          errors()
    { }

    iterator_compare & operator ++ ()
    {
        if (current_value != *compiter)
            errors.push_back(error_type(current_value, *compiter));
        ++compiter;
        return *this;
    }

    bool empty() const { return compiter.empty(); }
    ValueType & operator * () { return current_value; }

    unsigned_type get_num_errors() { return errors.size(); }
    vector<error_type> & get_errors() { return errors; }
};

int main(int argc, char **argv)
{
    const int small_block_order = 32; // must be a multiple of 32, assuming at least 4 bytes element size
    const int block_order = 32; // must be a multiple of 32, assuming at least 4 bytes element size

    int test_case = -1;
    int rank = 10000;
    int internal_memory_megabyte = 256;
    int mult_algo_num = 2;
    int sched_algo_num = 1;
    int internal_memory_byte = 0;

    dsr::Argument_helper ah;
    ah.new_int("K", "number of the test case to run", test_case);
    ah.new_named_int('r',  "rank", "N","rank of the matrices   default", rank);
    ah.new_named_int('m', "memory", "L", "internal memory to use (in megabytes)   default", internal_memory_megabyte);
    ah.new_named_int('a', "mult-algo", "N", "use multiplication-algorithm number N\n  available are:\n   0: naive_multiply_and_add\n   1: recursive_multiply_and_add\n   2: strassen_winograd_multiply_and_add\n   3: multi_level_strassen_winograd_multiply_and_add\n   4: strassen_winograd_multiply (block-interleaved pre- and postadditions)\n   5: strassen_winograd_multiply_and_add_interleaved (block-interleaved preadditions)\n   6: multi_level_strassen_winograd_multiply_and_add_block_grained\n  default", mult_algo_num);
    ah.new_named_int('s', "scheduling-algo", "N", "use scheduling-algorithm number N\n  available are:\n   0: online LRU\n   1: offline LFD\n   2: offline LRU prefetching\n  default", sched_algo_num);
    ah.new_named_int('c', "memory-byte", "L", "internal memory to use (in bytes)   no default", internal_memory_byte);

    ah.set_description("stxxl matrix test");
    ah.set_author("Raoul Steffen, R-Steffen@gmx.de");
    ah.process(argc, argv);

    int_type internal_memory;
    if (internal_memory_byte)
        internal_memory = internal_memory_byte;
    else
        internal_memory = int_type(internal_memory_megabyte) * 1048576;

    switch (test_case)
    {
    default:
        STXXL_ERRMSG("unknown testcase");
    case 4:
    {
        STXXL_MSG("multiplying two int_type matrices of rank " << rank << " block order " << small_block_order);
        typedef int_type value_type;

        typedef block_scheduler< matrix_swappable_block<value_type, small_block_order> > bst;
        typedef matrix<value_type, small_block_order> mt;
        typedef mt::row_vector_type rvt;
        typedef mt::column_vector_type cvt;
        typedef mt::row_major_iterator mitt;
        typedef mt::const_row_major_iterator cmitt;

        bst * b_s = new bst(internal_memory); // the block_scheduler may use internal_memory byte for caching
        //bst * b_s = new bst(16*sizeof(value_type)*small_block_order*small_block_order); // the block_scheduler may use 16 blocks for caching
        bst & bs = *b_s;
        mt * a = new mt(bs, rank, rank),
           * b = new mt(bs, rank, rank),
           * c = new mt(bs, rank, rank);
        stats_data stats_before, stats_after;
        matrix_operation_statistic_data matrix_stats_before, matrix_stats_after;

        // ------ first run
        for (mitt mit = a->begin(); mit != a->end(); ++mit)
            *mit = 1;
        for (mitt mit = b->begin(); mit != b->end(); ++mit)
            *mit = 1;

        bs.flush();
        STXXL_MSG("start mult");
        matrix_stats_before.set();
        stats_before = *stats::get_instance();
        *c = *a * *b;
        bs.flush();
        stats_after = *stats::get_instance();
        matrix_stats_after.set();
        STXXL_MSG("end mult");

        STXXL_MSG(matrix_stats_after - matrix_stats_before);
        STXXL_MSG(stats_after - stats_before);
        {
            int_type num_err = 0;
            for (cmitt mit = c->cbegin(); mit != c->cend(); ++mit)
                num_err += (*mit != rank);
            if (num_err)
                STXXL_ERRMSG("c had " << num_err << " errors");
        }

        // ------ second run
        {
            int_type i = 1;
            for (mitt mit = a->begin(); mit != a->end(); ++mit, ++i)
                *mit = i;
        }
        {
            b->set_zero();
            mt::iterator mit = b->begin();
            for (int_type i = 0; i < b->get_height(); ++i)
            {
                mit.set_pos(i,i);
                *mit = 1;
            }
        }

        bs.flush();
        STXXL_MSG("start mult");
        matrix_stats_before.set();
        stats_before = *stats::get_instance();
        *c = *a * *b;
        bs.flush();
        stats_after = *stats::get_instance();
        matrix_stats_after.set();
        STXXL_MSG("end mult");

        *c *= 3;
        *c += *a;
        STXXL_MSG(matrix_stats_after - matrix_stats_before);
        STXXL_MSG(stats_after - stats_before);
        {
            int_type num_err = 0;
            int_type i = 1;
            for (cmitt mit = c->cbegin(); mit != c->cend(); ++mit, ++i)
                num_err += (*mit != (i * 4));
            if (num_err)
                STXXL_ERRMSG("c had " << num_err << " errors");
        }

        {
            cvt x(rank),
                y;
            int_type i = 0;
            for (cvt::iterator it = x.begin(); it != x.end(); ++it)
                *it = ++i;
            y = *b * x;
            y = y + x;
            y += x;
            y = y - x;
            y -= x;
            y = x * 5;
            y *= 5;

            rvt w(rank),
                z;
            i = 0;
            for (rvt::iterator it = w.begin(); it != w.end(); ++it)
                *it = ++i;
            z = w * *b;
            z = z + w;
            z += w;
            z = z - w;
            z -= w;
            z = w * 5;
            z *= 5;

            *a = mt(bs, x, w);

            value_type v;
            v = w * x;
            STXXL_UNUSED(v);
        }

        delete a;
        delete b;
        delete c;
        delete b_s;
        break;
    }
    case 5:
    {
        STXXL_MSG("multiplying two full double matrices of rank " << rank << ", block order " << block_order
                << " using " << internal_memory_megabyte << "MiB internal memory, multiplication-algo "
                << mult_algo_num << ", scheduling-algo " << sched_algo_num);

        typedef double value_type;

        typedef block_scheduler< matrix_swappable_block<value_type, block_order> > bst;
        typedef matrix<value_type, block_order> mt;
        typedef mt::row_major_iterator mitt;
        typedef mt::const_row_major_iterator cmitt;

        bst * b_s = new bst(internal_memory); // the block_scheduler may use internal_memory byte for caching
        bst & bs = *b_s;
        mt * a = new mt(bs, rank, rank),
           * b = new mt(bs, rank, rank),
           * c = new mt(bs, rank, rank);
        stats_data stats_before, stats_after;
        matrix_operation_statistic_data matrix_stats_before, matrix_stats_after;

        STXXL_MSG("writing input matrices");
        for (mitt mit = a->begin(); mit != a->end(); ++mit)
            *mit = 1;
        for (mitt mit = b->begin(); mit != b->end(); ++mit)
            *mit = 1;

        bs.flush();
        STXXL_MSG("start of multiplication");
        matrix_stats_before.set();
        stats_before = *stats::get_instance();
        if (mult_algo_num >= 0)
            *c = a->multiply(*b, mult_algo_num, sched_algo_num);
        else
            *c = a->multiply_internal(*b, sched_algo_num);
        bs.flush();
        stats_after = *stats::get_instance();
        matrix_stats_after.set();
        STXXL_MSG("end of multiplication");

        STXXL_MSG(matrix_stats_after - matrix_stats_before);
        STXXL_MSG(stats_after - stats_before);
        {
            int_type num_err = 0;
            for (cmitt mit = c->cbegin(); mit != c->cend(); ++mit)
                num_err += (*mit != rank);
            if (num_err)
                STXXL_ERRMSG("c had " << num_err << " errors");
        }

        delete a;
        delete b;
        delete c;
        delete b_s;
        break;
    }
    }
    STXXL_MSG("end of test");
    return 0;
}
