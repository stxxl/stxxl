/***************************************************************************
 *  containers/test_vector_sizes.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/vector>


typedef int my_type;
typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
typedef vector_type::block_type block_type;

void test(const char * fn, stxxl::unsigned_type sz, my_type ofs)
{
    {
        stxxl::syscall_file f(fn, stxxl::file::CREAT | stxxl::file::DIRECT | stxxl::file::RDWR);
        vector_type v(&f);
        v.resize(sz);
        STXXL_MSG("writing " << v.size() << " elements");
        for (stxxl::unsigned_type i = 0; i < v.size(); ++i)
            v[i] = ofs + i;
    }

    {
        stxxl::syscall_file f(fn, stxxl::file::DIRECT | stxxl::file::RDWR);
        vector_type v(&f);
        STXXL_MSG("reading " << v.size() << " elements (RDWR)");
        assert(v.size() == sz);
        for (stxxl::unsigned_type i = 0; i < v.size(); ++i)
            assert(v[i] == ofs + my_type(i));
    }

    {
        stxxl::syscall_file f(fn, stxxl::file::DIRECT | stxxl::file::RDONLY);
        vector_type v(&f);
        STXXL_MSG("reading " << v.size() << " elements (RDONLY)");
        assert(v.size() == sz);
        for (stxxl::unsigned_type i = 0; i < v.size(); ++i)
            assert(v[i] == ofs + my_type(i));
    }
}

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " file [filetype]" << std::endl;
        return -1;
    }

    const char * fn = argv[1];

    // multiple of block size
    test(argv[1], 4 * block_type::size, 100000000);

    // multiple of page size, but not block size
    test(argv[1], 4 * block_type::size + 4096, 200000000);

    // multiple of neither block size nor page size
    test(argv[1], 4 * block_type::size + 4096 + 23, 300000000);

    {
        stxxl::syscall_file f(fn, stxxl::file::DIRECT | stxxl::file::RDWR);
        f.set_size(f.size() - 1);
    }

    {
        stxxl::syscall_file f(fn, stxxl::file::DIRECT | stxxl::file::RDWR);
        vector_type v(&f);
        STXXL_MSG("reading " << v.size() << " elements (RDWR)");
        for (stxxl::unsigned_type i = 0; i < v.size(); ++i)
            assert(v[i] == 300000000 + my_type(i));
    }

    {
        stxxl::syscall_file f(fn, stxxl::file::DIRECT | stxxl::file::RDWR);
        f.set_size(f.size() - 1);
    }

    {
        stxxl::syscall_file f(fn, stxxl::file::DIRECT | stxxl::file::RDONLY);
        vector_type v(&f);
        STXXL_MSG("reading " << v.size() << " elements (RDONLY)");
        for (stxxl::unsigned_type i = 0; i < v.size(); ++i)
            assert(v[i] == 300000000 + my_type(i));
    }
}
// vim: et:ts=4:sw=4
