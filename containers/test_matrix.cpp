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

#include <stxxl/bits/containers/matrix.h>
#include <stxxl/vector>
#include <stxxl/stream>

// Thanks Daniel Russel, Stanford University
#include <Argument_helper.h>

#include <iostream>
#include <limits>

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
    const int small_block_order = 64; // must be a multiple of 64
    const int block_order = 2048; // must be a multiple of 64

    int test_case = -1;
    int rank = 10000;
    int internal_memory_megabyte = 256;

    dsr::Argument_helper ah;
    ah.new_int("K", "number of the test case to run", test_case);
    ah.new_named_int('r',  "rank", "N","rank of the matrices", rank);
    ah.new_named_int('m', "memory", "L", "internal memory to use (in megabytes)", internal_memory_megabyte);

    ah.set_description("stxxl matrix test");
    ah.set_author("Raoul Steffen, R-Steffen@gmx.de");
    ah.process(argc, argv);

    int_type internal_memory = int_type(internal_memory_megabyte) * 1048576;

    switch (test_case)
    {
    case 0:
    {
        blocked_matrix<unsigned_type, block_order> A(rank, rank);
        modulus_integers mi1(1, 1), mi2(1, 1);
        iterator_compare<modulus_integers, unsigned_type> ic(mi1);

        STXXL_MSG("Writing and reading " << rank << "x" << rank << " matrix.");
        A.materialize_from_row_major(mi2, internal_memory);
        A.output_to_row_major(ic, internal_memory);
        STXXL_MSG("" << ic.get_errors().size() << " errors");
        if (ic.get_errors().size() != 0)
        {
            STXXL_MSG("first errors:");
            for (unsigned int i = 0; i < 10 && i < ic.get_errors().size(); i++)
                STXXL_MSG("" << ic.get_errors()[i].first << " " << ic.get_errors()[i].second);
        }
        break;
    }
    case 1:
    {   //1-matrices
        ColumnMajor cm;
        blocked_matrix<double, block_order> A(rank, rank), B(rank, rank), C(rank, rank);
        constant_one co;
        modulus_integers mi(rank, 0);   //expected result, no modulus
        iterator_compare<modulus_integers, unsigned_type> ic(mi);

        stats_data stats_before_construction(*stats::get_instance());

        A.materialize_from_row_major(co, internal_memory);
        B.materialize_from_row_major(co, internal_memory);

        stats_data stats_before_mult(*stats::get_instance());
        STXXL_MSG(stats_before_mult - stats_before_construction);

        STXXL_MSG("Multiplying two " << rank << "x" << rank << " matrices.");
        multiply(A, B, C, internal_memory);

        stats_data stats_after(*stats::get_instance());
        STXXL_MSG(stats_after - stats_before_mult);

        STXXL_MSG("testing correctness");
        C.output_to_row_major(ic, internal_memory);
        STXXL_MSG("" << ic.get_errors().size() << " errors");
        if (ic.get_errors().size() != 0)
        {
            STXXL_MSG("first errors:");
            for (unsigned int i = 0; i < 10 && i < ic.get_errors().size(); i++)
                STXXL_MSG("" << ic.get_errors()[i].first << " " << ic.get_errors()[i].second);
        }
        break;
    }
    case 2:
    {   //ascending times constant factor
        blocked_matrix<unsigned_type, block_order> A(rank, rank), B(rank, rank), C(rank, rank);
        modulus_integers mi1(1, 1), mi2(5, 5);  //ascending, 5 * ascending
        diagonal_matrix di(rank, 5);    //multiply by 5
        iterator_compare<modulus_integers, unsigned_type> ic(mi2);

        A.materialize_from_row_major(mi1, internal_memory);
        B.materialize_from_row_major(di, internal_memory);

        STXXL_MSG("Multiplying two " << rank << "x" << rank << " matrices.");
        stats_data stats_before(*stats::get_instance());

        multiply(A, B, C, internal_memory);

        stats_data stats_after(*stats::get_instance());
        STXXL_MSG(stats_after - stats_before);

        STXXL_MSG("testing correctness");
        C.output_to_row_major(ic, internal_memory);
        STXXL_MSG("" << ic.get_errors().size() << " errors");
        if (ic.get_errors().size() != 0)
        {
            STXXL_MSG("first errors:");
            for (unsigned int i = 0; i < 10 && i < ic.get_errors().size(); i++)
                STXXL_MSG("" << ic.get_errors()[i].first << " " << ic.get_errors()[i].second);
        }
        break;
    }
    case 3:
    {
        blocked_matrix<unsigned_type, block_order> A(rank, rank), B(rank, rank), C(rank, rank);
        modulus_integers mi1(1, 1), mi2((unsigned_type)rank * rank, std::numeric_limits<unsigned_type>::max());
        inverse_diagonal_matrix id(rank);
        iterator_compare<modulus_integers, unsigned_type> ic(mi2);

        A.materialize_from_row_major(mi1, internal_memory);
        B.materialize_from_row_major(id, internal_memory);

        STXXL_MSG("Multiplying two " << rank << "x" << rank << " matrices twice.");
        stats_data stats_before(*stats::get_instance());

        multiply(A, B, C, internal_memory);
        multiply(B, C, A, internal_memory);

        stats_data stats_after(*stats::get_instance());
        STXXL_MSG(stats_after - stats_before);

        STXXL_MSG("testing correctness");
        A.output_to_row_major(ic, internal_memory);
        STXXL_MSG("" << ic.get_errors().size() << " errors");
        if (ic.get_errors().size() != 0)
        {
            STXXL_MSG("first errors:");
            for (unsigned int i = 0; i < 10 && i < ic.get_errors().size(); i++)
                STXXL_MSG("" << ic.get_errors()[i].first << " != " << ic.get_errors()[i].second);
        }
        break;
    }
    case 4:
    {
        STXXL_MSG("multiplying two int_type matrices of rank " << rank << " block order " << small_block_order);
        typedef int_type value_type;

        typedef block_scheduler< matrix_swappable_block<value_type, small_block_order> > bst;
        typedef matrix<value_type, small_block_order> mt;
        typedef mt::row_vector_type rvt;
        typedef mt::column_vector_type cvt;
        typedef mt::row_major_iterator mitt;

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
            for (mitt mit = c->begin(); mit != c->end(); ++mit)
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

        STXXL_MSG(matrix_stats_after - matrix_stats_before);
        STXXL_MSG(stats_after - stats_before);
        *c *= 3;
        *c += *a;
        {
            int_type num_err = 0;
            int_type i = 1;
            for (mitt mit = c->begin(); mit != c->end(); ++mit, ++i)
                num_err += (*mit != i * 3);
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
        }

        delete a;
        delete b;
        delete c;
        delete b_s;
        break;
    }
    case 5:
    {
        STXXL_MSG("multiplying two double matrices of rank " << rank << " block order " << block_order);

        typedef double value_type;

        typedef block_scheduler< matrix_swappable_block<value_type, block_order> > bst;
        typedef matrix<value_type, block_order> mt;
        typedef mt::row_major_iterator mitt;

        bst * b_s = new bst(internal_memory); // the block_scheduler may use internal_memory byte for caching
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
        STXXL_MSG("start of first run (full matrices)");
        matrix_stats_before.set();
        stats_before = *stats::get_instance();
        *c = *a * *b;
        bs.flush();
        stats_after = *stats::get_instance();
        matrix_stats_after.set();
        STXXL_MSG("end of first run");

        STXXL_MSG(matrix_stats_after - matrix_stats_before);
        STXXL_MSG(stats_after - stats_before);
        {
            int_type num_err = 0;
            for (mitt mit = c->begin(); mit != c->end(); ++mit)
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
        STXXL_MSG("start of second run (one matrix is diagonal)");
        matrix_stats_before.set();
        stats_before = *stats::get_instance();
        *c = *a * *b;
        bs.flush();
        stats_after = *stats::get_instance();
        matrix_stats_after.set();
        STXXL_MSG("end of second run");

        STXXL_MSG(matrix_stats_after - matrix_stats_before);
        STXXL_MSG(stats_after - stats_before);
        {
            int_type num_err = 0;
            int_type i = 1;
            for (mitt mit = a->begin(); mit != a->end(); ++mit, ++i)
                num_err += (*mit != i);
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

// vim: et:ts=4:sw=4
