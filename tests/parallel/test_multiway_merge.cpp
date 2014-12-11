/***************************************************************************
 *  tests/parallel/test_multiway_merge.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/parallel/multiway_merge.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/random>
#include <iostream>

struct Something
{
    int a, b;

    Something(int x = 0)
        : a(x), b(x * x)
    { }

    bool operator < (const Something& other) const
    {
        return a < other.a;
    }

    friend std::ostream& operator << (std::ostream& os, const Something& s)
    {
        return os << '(' << s.a << ',' << s.b << ')';
    }
};

template class stxxl::parallel::LoserTreeExplicit<Something>;
template class stxxl::parallel::LoserTree<Something>;
template class stxxl::parallel::LoserTreeReference<Something>;
template class stxxl::parallel::LoserTreePointer<Something>;
template class stxxl::parallel::LoserTreeUnguarded<Something>;
template class stxxl::parallel::LoserTreePointerUnguarded<Something>;

void test_vecs(unsigned int vecnum)
{
    static const bool debug = false;

    std::cout << "testing winner_tree with " << vecnum << " players\n";

    // construct many vectors of sorted random numbers

    stxxl::random_number32 rnd;
    std::vector<std::vector<unsigned int> > vec(vecnum);

    size_t totalsize = 0;
    std::vector<unsigned int> output, correct;

    for (size_t i = 0; i < vecnum; ++i)
    {
        // determine number of items in stream
        size_t inum = (rnd() % 128) + 64;
        vec[i].resize(inum);
        totalsize += inum;

        // pick random items
        for (size_t j = 0; j < inum; ++j)
            vec[i][j] = (unsigned int)(rnd() % (vecnum * 20));

        std::sort(vec[i].begin(), vec[i].end());

        // put items into correct vector as well
        correct.insert(correct.end(), vec[i].begin(), vec[i].end());
    }

    if (debug)
    {
        for (size_t i = 0; i < vecnum; ++i)
        {
            std::cout << "vec[" << i << "]:";
            for (size_t j = 0; j < vec[i].size(); ++j)
                std::cout << " " << vec[i][j];
            std::cout << "\n";
        }
    }

    // prepare output and correct vector
    output.resize(totalsize);
    std::sort(correct.begin(), correct.end());

    // construct vector of input iterator ranges
    typedef std::vector<unsigned int>::iterator input_iterator;

    std::vector<std::pair<input_iterator, input_iterator> > sequences(vecnum);

    for (size_t i = 0; i < vecnum; ++i)
    {
        sequences[i] = std::make_pair(vec[i].begin(), vec[i].end());
    }

    stxxl::parallel::multiway_merge(sequences.begin(), sequences.end(),
                                    output.begin(), totalsize,
                                    std::less<Something>(),
                                    false);

    STXXL_CHECK(output.size() == totalsize);

    if (debug)
    {
        for (size_t i = 0; i < output.size(); ++i)
            std::cout << output[i] << " ";
    }

    STXXL_CHECK(output == correct);
}

int main()
{
    // run multiway merge tests for 0..256 sequences
    for (unsigned int i = 0; i <= 256; ++i)
        test_vecs(i);

    return 0;
}
