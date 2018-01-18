/***************************************************************************
 *  tests/containers/test_vector_sizes.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/io.hpp>

#include <stxxl/vector>

using my_type = int;
using vector_type = stxxl::vector<my_type>;
using block_type = vector_type::block_type;

void test_write(const char* fn, const char* ft, size_t sz, my_type ofs)
{
    foxxll::file_ptr f = foxxll::create_file(
        ft, fn, foxxll::file::CREAT | foxxll::file::DIRECT | foxxll::file::RDWR);
    {
        vector_type v(f);
        v.resize(sz);
        LOG1 << "writing " << v.size() << " elements";
        for (size_t i = 0; i < v.size(); ++i)
            v[i] = ofs + (int)i;
    }
}

template <typename Vector>
void test_rdwr(const char* fn, const char* ft, size_t sz, my_type ofs)
{
    foxxll::file_ptr f = foxxll::create_file(
        ft, fn, foxxll::file::DIRECT | foxxll::file::RDWR);
    {
        Vector v(f);
        LOG1 << "reading " << v.size() << " elements (RDWR)";
        die_unless(v.size() == sz);
        for (size_t i = 0; i < v.size(); ++i)
            die_unless(v[i] == ofs + my_type(i));
    }
}

template <typename Vector>
void test_rdonly(const char* fn, const char* ft, size_t sz, my_type ofs)
{
    foxxll::file_ptr f = foxxll::create_file(
        ft, fn, foxxll::file::DIRECT | foxxll::file::RDONLY);
    {
        Vector v(f);
        LOG1 << "reading " << v.size() << " elements (RDONLY)";
        die_unless(v.size() == sz);
        for (size_t i = 0; i < v.size(); ++i)
            die_unless(v[i] == ofs + my_type(i));
    }
}

void test(const char* fn, const char* ft, size_t sz, my_type ofs)
{
    test_write(fn, ft, sz, ofs);
    test_rdwr<const vector_type>(fn, ft, sz, ofs);
    test_rdwr<vector_type>(fn, ft, sz, ofs);

    // 2013-tb: there is a bug with read-only vectors on mmap backed files:
    // copying from mmapped area will fail for invalid ranges at the end,
    // whereas a usual read() will just stop short at the end. The read-only
    // vector however will always read the last block in full, thus causing a
    // segfault with mmap files. FIXME
    if (strcmp(ft, "mmap") == 0) return;

    test_rdonly<const vector_type>(fn, ft, sz, ofs);
    //-tb: vector always writes data! FIXME
    //test_rdonly<vector_type>(fn, ft, sz, ofs);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " file [filetype]" << std::endl;
        return -1;
    }

    foxxll::config::get_instance();

    const char* fn = argv[1];
    const char* ft = (argc >= 3) ? argv[2] : "syscall";

    size_t start_elements = 42 * block_type::size;

    LOG1 << "using " << ft << " file";

    // multiple of block size
    LOG1 << "running test with " << start_elements << " items";
    test(fn, ft, start_elements, 100000);

    // multiple of page size, but not block size
    LOG1 << "running test with " << start_elements << " + 4096 items";
    test(fn, ft, start_elements + 4096, 200000);

    // multiple of neither block size nor page size
    LOG1 << "running test with " << start_elements << " + 4096 + 23 items";
    test(fn, ft, start_elements + 4096 + 23, 300000);

    // truncate 1 byte
    {
        foxxll::syscall_file f(fn, foxxll::file::DIRECT | foxxll::file::RDWR);
        LOG1 << "file size is " << f.size() << " bytes";
        f.set_size(f.size() - 1);
        LOG1 << "truncated to " << f.size() << " bytes";
    }

    // will truncate after the last complete element
    test_rdwr<vector_type>(fn, ft, start_elements + 4096 + 23 - 1, 300000);

    // truncate 1 more byte
    {
        foxxll::syscall_file f(fn, foxxll::file::DIRECT | foxxll::file::RDWR);
        LOG1 << "file size is " << f.size() << " bytes";
        f.set_size(f.size() - 1);
        LOG1 << "truncated to " << f.size() << " bytes";
    }

    // will not truncate
    //-tb: vector already writes data! TODO
    //test_rdonly<vector_type>(fn, ft, start_elements + 4096 + 23 - 2, 300000);

    // check final size
    {
        foxxll::syscall_file f(fn, foxxll::file::DIRECT | foxxll::file::RDWR);
        LOG1 << "file size is " << f.size() << " bytes";
        die_unless(f.size() == (start_elements + 4096 + 23 - 1) * sizeof(my_type) - 1);
    }

    {
        foxxll::syscall_file f(fn, foxxll::file::DIRECT | foxxll::file::RDWR);
        f.close_remove();
    }
}
// vim: et:ts=4:sw=4
