/***************************************************************************
 *  mng/test_aligned.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_VERBOSE_LEVEL 0
#define STXXL_VERBOSE_ALIGNED_ALLOC STXXL_VERBOSE0
#define STXXL_VERBOSE_TYPED_BLOCK STXXL_VERBOSE0

#include <iostream>
#include <vector>
#include <stxxl/mng>

#define BLOCK_SIZE (1024 * 1024)

struct type
{
    int i;
    ~type() { }
};

typedef stxxl::typed_block<BLOCK_SIZE, type> block_type;

void test_typed_block()
{
    block_type * a = new block_type;
    block_type * b = new block_type;
    block_type * A = new block_type[4];
    block_type * B = new block_type[1];
    block_type * C = NULL;
    //C = new block_type[0];
    delete a;
    a = b;
    b = 0;
    delete b;
    delete a;
    delete[] A;
    delete[] B;
    delete[] C;
}

void test_aligned_alloc()
{
    void * p = stxxl::aligned_alloc<1024>(4096);
    void * q = NULL;
    void * r = stxxl::aligned_alloc<1024>(4096, 42);
    stxxl::aligned_dealloc<1024>(p);
    stxxl::aligned_dealloc<1024>(q);
    stxxl::aligned_dealloc<1024>(r);
}

void test_typed_block_vector()
{
    std::vector<block_type> v1(2);
    std::vector<block_type, stxxl::new_alloc<block_type> > v2(2);
}

int main()
{
    try {
        test_typed_block();
        test_aligned_alloc();
        test_typed_block_vector();
    } catch (std::exception e) {
        STXXL_MSG("OOPS: " << e.what());
        return 1;
    } catch (char const * c) {
        STXXL_MSG("OOPS: " << c);
        return 1;
    }
}
// vim: et:ts=4:sw=4
