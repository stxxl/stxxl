/***************************************************************************
 *  algo/sort_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example algo/sort_file.cpp
//! This example imports a file into an \c stxxl::vector without copying its
//! content and then sorts it using stxxl::sort / stxxl::ksort / ...

#include <stxxl/io>
#include <stxxl/mng>
#include <stxxl/ksort>
#include <stxxl/sort>
#include <stxxl/stable_ksort>
#include <stxxl/vector>


struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    char _data[128 - sizeof(key_type)];
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
};


inline bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}

inline bool operator == (const my_type & a, const my_type & b)
{
    return a.key() == b.key();
}

struct Cmp
{
    bool operator () (const my_type & a, const my_type & b) const
    {
        return a < b;
    }
    static my_type min_value()
    {
        return my_type::min_value();
    }
    static my_type max_value()
    {
        return my_type::max_value();
    }
};

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj._key;
    return o;
}

int main(int argc, char ** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " action file" << std::endl;
        std::cout << "       where action is one of generate, sort, ksort, stable_sort, stable_ksort" << std::endl;
        return -1;
    }

    if (strcasecmp(argv[1], "generate") == 0) {
        const my_type::key_type max_key = 1 * 1024 * 1024;
        const unsigned int block_size = 1 * 1024 * 1024;
        const unsigned int records_in_block = block_size / sizeof(my_type);
        stxxl::syscall_file f(argv[2], stxxl::file::CREAT | stxxl::file::RDWR);
        my_type * array = (my_type *)stxxl::aligned_alloc<BLOCK_ALIGN>(block_size);
        memset(array, 0, block_size);

        my_type::key_type cur_key = max_key;
        for (unsigned i = 0; i < max_key / records_in_block; i++)
        {
            for (unsigned j = 0; j < records_in_block; j++)
                array[j]._key = cur_key--;

            stxxl::request_ptr req = f.awrite((void *)array, stxxl::int64(i) * block_size, block_size, stxxl::default_completion_handler());
            req->wait();
        }
        stxxl::aligned_dealloc<BLOCK_ALIGN>(array);
    } else {
        stxxl::syscall_file f(argv[2], stxxl::file::DIRECT | stxxl::file::RDWR);
        unsigned memory_to_use = 50 * 1024 * 1024;
        typedef stxxl::vector<my_type> vector_type;
        vector_type v(&f);

        /*
        STXXL_MSG("Printing...");
        for(stxxl::int64 i=0; i < v.size(); i++)
            STXXL_MSG(v[i].key());
         */

        STXXL_MSG("Checking order...");
        STXXL_MSG((stxxl::is_sorted(v.begin(), v.end()) ? "OK" : "WRONG"));

        STXXL_MSG("Sorting...");
        if (strcasecmp(argv[1], "sort") == 0) {
            stxxl::sort(v.begin(), v.end(), Cmp(), memory_to_use);
        /* stable_sort is not yet implemented
        } else if (strcasecmp(argv[1], "stable_sort") == 0) {
            stxxl::stable_sort(v.begin(), v.end(), memory_to_use);
        */
        } else if (strcasecmp(argv[1], "ksort") == 0) {
            stxxl::ksort(v.begin(), v.end(), memory_to_use);
        } else if (strcasecmp(argv[1], "stable_ksort") == 0) {
            stxxl::stable_ksort(v.begin(), v.end(), memory_to_use);
        } else {
            STXXL_MSG("Not implemented: " << argv[1]);
        }

        STXXL_MSG("Checking order...");
        STXXL_MSG((stxxl::is_sorted(v.begin(), v.end()) ? "OK" : "WRONG"));
    }

    return 0;
}

// vim: et:ts=4:sw=4
