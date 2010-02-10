/***************************************************************************
 *  containers/test_vector.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2003, 2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/test_vector.cpp
//! This is an example of use of \c stxxl::vector and
//! \c stxxl::VECTOR_GENERATOR. Vector type is configured
//! to store 64-bit integers and have 2 pages each of 1 block

#include <iostream>
#include <algorithm>
#include <stxxl/vector>
#include <stxxl/scan>

struct element	//24 bytes, not a power of 2 intentionally
{
	stxxl::int64 key;
	stxxl::int64 load0;
	stxxl::int64 load1;

	element& operator=(stxxl::int64 i)
	{
		key = i;
		return *this;
	}
};

struct counter
{
    int value;
    counter(int v) : value(v) { }
    int operator () ()
    {
        int old_val = value;
        value++;
        return old_val;
    }
};

template <class my_vec_type>
void test_const_iterator(const my_vec_type & x)
{
    typename my_vec_type::const_iterator i = x.begin();
    i = x.end() - 1;
    i.block_externally_updated();
    i.flush();
    i++;
    ++i;
    --i;
    i--;
    *i;
}


int main()
{
    try
    {
        // use non-randomized striping to avoid side effects on random generator
        typedef stxxl::VECTOR_GENERATOR<element, 2, 2, (2 * 1024 * 1024), stxxl::striping>::result vector_type;
        vector_type v(64 * 1024 * 1024 / sizeof(element));

        // test assignment const_iterator = iterator
        vector_type::const_iterator c_it = v.begin();

        unsigned int big_size = 1024 * 1024 * 2 * 16 * 16;
        typedef stxxl::vector<double> vec_big;
        vec_big my_vec(big_size);

        vec_big::iterator big_it = my_vec.begin();
        big_it += 6;

        test_const_iterator(v);

        stxxl::random_number32 rnd;
        int offset = rnd();

        STXXL_MSG("write " << v.size() << " elements");

        stxxl::ran32State = 0xdeadbeef;
        vector_type::size_type i;

        // fill the vector with increasing sequence of integer numbers
        for (i = 0; i < v.size(); ++i)
        {
            v[i].key = i + offset;
            assert(v[i].key == stxxl::int64(i + offset));
        }


        // fill the vector with random numbers
        stxxl::generate(v.begin(), v.end(), stxxl::random_number32(), 4);
        v.flush();

        STXXL_MSG("seq read of " << v.size() << " elements");

        stxxl::ran32State = 0xdeadbeef;

        // testing swap
        vector_type a;
        std::swap(v, a);
        std::swap(v, a);

        for (i = 0; i < v.size(); i++)
        {
            assert(v[i].key == rnd());
        }

        // check again
        STXXL_MSG("clear");

        v.clear();

        stxxl::ran32State = 0xdeadbeef + 10;

        v.resize(64 * 1024 * 1024 / sizeof(element));

        STXXL_MSG("write " << v.size() << " elements");
        stxxl::generate(v.begin(), v.end(), stxxl::random_number32(), 4);

        stxxl::ran32State = 0xdeadbeef + 10;

        STXXL_MSG("seq read of " << v.size() << " elements");

        for (i = 0; i < v.size(); i++)
        {
            assert(v[i].key == rnd());
        }

        std::vector<stxxl::vector<int> > vector_of_stxxlvectors(2);
        // test copy operator
        vector_of_stxxlvectors[0] = vector_of_stxxlvectors[1];

        assert(vector_of_stxxlvectors[0] == vector_of_stxxlvectors[1]);
    }
    catch (const std::exception & ex)
    {
        STXXL_MSG("Caught exception: " << ex.what());
    }
    catch (...)
    {
        STXXL_MSG("Caught unknown exception.");
    }

    return 0;
}
