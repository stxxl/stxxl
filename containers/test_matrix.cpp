/***************************************************************************
 *  include/stxxl/bits/containers/test_matrix.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/containers/matrix.h>
#include <stxxl/vector>
#include <stxxl/stream>

#include <time.h>
#include <iostream>

using namespace stxxl;

struct constant_one
{
	const constant_one& operator++() const { return *this; }
	bool empty() const { return false; }
	int operator*() const { return 1; }
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
    
    modulus_integers& operator++()
    {
        counter += step;
        if (modulus != 0)
            counter %= modulus;
        return *this;
    }
    
    bool empty() const { return false; }
    
    unsigned_type operator*() const { return counter; }
};

struct diagonal_matrix
{
private:
    unsigned_type order, counter, value;
    
public:
    diagonal_matrix(unsigned_type order, unsigned_type value = 1)
        : order(order), counter(0), value(value) { }
    
    diagonal_matrix& operator++()
    {
        ++counter;
        return *this;
    }
    
    bool empty() const { return false; }
    
    unsigned_type operator*() const
    { return (counter % (order+1) == 0) * value; }
};

struct inverse_diagonal_matrix
{
private:
    unsigned_type order, counter, value;
    
public:
    inverse_diagonal_matrix(unsigned_type order, unsigned_type value = 1)
        : order(order), counter(0), value(value) { }
    
    inverse_diagonal_matrix& operator++()
    {
        ++counter;
        return *this;
    }
    
    bool empty() const { return false; }
    
    unsigned_type operator*() const
    { return (counter % order == order-1 - counter / order) * value; }
};

template<class CompareIterator, typename ValueType>
class iterator_compare
{
    typedef std::pair<ValueType, ValueType> error_type;
    
    CompareIterator& compiter;
    ValueType current_value;
    vector<error_type> errors;
public:
    iterator_compare(CompareIterator& co)
        : compiter(co),
            current_value(),
            errors()
    { }
    
    iterator_compare& operator++()
    {
        if (current_value != *compiter)
            errors.push_back(error_type(current_value, *compiter));
        ++compiter;
        return *this;
    }
    
    bool empty() const { return compiter.empty(); }
    ValueType& operator*() { return current_value; }
    
    unsigned_type get_num_errors() { return errors.size(); }
    vector<error_type>& get_errors() { return errors; }
};

int main()
{
    const int rank = 100 * 1024, block_order = 2 * 1024;
    
    const unsigned_type internal_memory = 4ull * 1024 * 1024 * 1024;
    
    switch (1)
    {
    case 0:
    {
        matrix<unsigned_type, block_order> A(rank, rank);
        modulus_integers mi1(1, 1), mi2(1,1);
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
    {
        matrix<double, block_order> A(rank, rank), B(rank, rank), C(rank, rank);
        constant_one co;
        modulus_integers mi(rank, 0);
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
    {
        matrix<unsigned_type, block_order> A(rank, rank), B(rank, rank), C(rank, rank);
        modulus_integers mi1(1, 1), mi2(5, 5);
        diagonal_matrix di(rank,5);
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
        matrix<unsigned_type, block_order> A(rank, rank), B(rank, rank), C(rank, rank);
        modulus_integers mi1(1, 1), mi2((unsigned_type)rank*rank, -1);
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
        STXXL_MSG("" << -1 / 3 << " " << -5 / 3);
        break;
    }
    }
    return 0;
}
