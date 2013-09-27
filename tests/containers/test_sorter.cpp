/***************************************************************************
 *  tests/container/test_sorter.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2012 Timo Bingmann <bingmann@kit.edu>
 *  - based on algo/test_sort.cpp
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/test_sorter.cpp
//! This is an example of how to use \c stxxl::sorter() container

#include <limits>
#include <stxxl/sorter>

#define RECORD_SIZE 8

struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    char _data[RECORD_SIZE - sizeof(key_type)];
    key_type key() const
    {
        return _key;
    }

    my_type() { }
    my_type(key_type __key) : _key(__key) { }

    static my_type min_value()
    {
        return my_type((std::numeric_limits<key_type>::min)());
    }
    static my_type max_value()
    {
        return my_type((std::numeric_limits<key_type>::max)());
    }

    ~my_type() { }
};

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj._key;
    return o;
}

bool operator == (const my_type & a, const my_type & b)
{
    return a.key() == b.key();
}

bool operator <= (const my_type & a, const my_type & b)
{
    return a.key() <= b.key();
}

struct Comparator : public std::less<my_type>
{
    inline bool operator()(const my_type & a, const my_type & b) const
    {
        return a.key() < b.key();
    }
    my_type min_value() const
    {
        return my_type::min_value();
    }
    my_type max_value() const
    {
        return my_type::max_value();
    }
};


int main()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE
    STXXL_MSG("STXXL_PARALLEL_MULTIWAY_MERGE");
#endif
    unsigned memory_to_use = 128 * 1024 * 1024;
    enum { block_size = 8192 };

    // comparator object used for sorters
    Comparator cmp;

    // construct sorter type: stxxl::sorter<ValueType, ComparatorType, BlockSize>
    typedef stxxl::sorter<my_type, Comparator, block_size> sorter_type;

    {
        // test small number of items that can be sorted internally

        sorter_type s (cmp, memory_to_use);

        // put in some items
        s.push(42);
        s.push(0);
        s.push(23);

        // finish input, switch to sorting stage.
        s.sort();

        assert( *s == 0 );      ++s;
        assert( *s == 23 );     ++s;
        assert( *s == 42 );     ++s;
        assert( s.empty() );
    }

    {
        // large test with 384 MiB items

        const stxxl::uint64 n_records = stxxl::int64(384) * stxxl::int64(1024 * 1024) / sizeof(my_type);

        sorter_type s (cmp, memory_to_use);

        stxxl::random_number32 rnd;

        STXXL_MSG("Filling sorter..., input size = " << n_records << " elements (" << ((n_records * sizeof(my_type)) >> 20) << " MiB)");

        for (stxxl::uint64 i = 0; i < n_records; i++)
        {
            assert( s.size() == i );

            s.push( 1 + (rnd() % 0xfffffff) );
        }

        // finish input, switch to sorting stage.
        s.sort();

        STXXL_MSG("Checking order...");

        assert( !s.empty() );
        assert( s.size() == n_records );

        my_type prev = *s;      // get first item
        ++s;

        stxxl::uint64 count = n_records - 1;

        while ( !s.empty() )
        {
            assert( s.size() == count );

            if ( !(prev <= *s) ) STXXL_MSG("WRONG");
            assert( prev <= *s );

            ++s; --count;
        }
        STXXL_MSG("OK");

        // rewind and read output again
        s.rewind();

        STXXL_MSG("Checking order again...");
        
        assert( !s.empty() );
        assert( s.size() == n_records );

        prev = *s;      // get first item
        ++s;

        while ( !s.empty() )
        {
            if ( !(prev <= *s) ) STXXL_MSG("WRONG");
            assert( prev <= *s );

            ++s;
        }
        STXXL_MSG("OK");

        assert( s.size() == 0 );

        STXXL_MSG("Done");
    }

    return 0;
}

// vim: et:ts=4:sw=4
