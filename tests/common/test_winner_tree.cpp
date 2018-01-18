/***************************************************************************
 *  tests/common/test_winner_tree.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <algorithm>
#include <iostream>
#include <vector>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/types.hpp>

#include <stxxl/bits/common/winner_tree.h>
#include <stxxl/random>

//! Comparator interface for the winner tree: takes two players and decides
//! which is smaller. In this implementation the players are the top elements
//! from an array of vectors.
struct VectorCompare
{
    //! number of vectors in comparator
    size_t m_vecnum;
    //! vector of pointers to player streams
    const std::vector<std::vector<size_t> >& m_vec;

    //! currently top indices
    std::vector<size_t> m_vecp;

    VectorCompare(size_t vecnum,
                  const std::vector<std::vector<size_t> >& vec)
        : m_vecnum(vecnum), m_vec(vec), m_vecp(vecnum, 0)
    { }

    //! perform a game with player v1 and v2.
    inline bool operator () (size_t v1, size_t v2) const
    {
        return m_vec[v1][m_vecp[v1]] < m_vec[v2][m_vecp[v2]];
    }
};

// forced instantiation
template class stxxl::winner_tree<VectorCompare>;

//! Run tests for a specific number of vectors
void test_vecs(size_t vecnum, bool test_rebuild)
{
    static const bool debug = false;

    std::cout << "testing winner_tree with " << vecnum << " players using "
              << (test_rebuild ? "replay_on_pop()" : "rebuild()") << "\n";

    // construct many vectors of sorted random numbers

    stxxl::random_number32 rnd;
    std::vector<std::vector<size_t> > vec(vecnum);

    size_t totalsize = 0;
    std::vector<size_t> output, correct;

    for (size_t i = 0; i < vecnum; ++i)
    {
        // determine number of items in stream
        size_t inum = static_cast<size_t>((rnd() % 128) + 64);
        vec[i].resize(inum);
        totalsize += inum;

        // pick random items
        for (size_t j = 0; j < inum; ++j)
            vec[i][j] = static_cast<size_t>(rnd() % (vecnum * 20));

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
    output.reserve(totalsize);
    std::sort(correct.begin(), correct.end());

    VectorCompare vc(vecnum, vec);
    stxxl::winner_tree<VectorCompare> wt(1, vc);

    for (size_t i = 0; i < vecnum; ++i)
    {
        if (vec[i].size())
            wt.activate_player(i);
    }

    if (debug) std::cout << "after activation: " << wt.to_string();

    while (!wt.empty())
    {
        // get winner
        size_t top = wt.top();

        // copy winner's item to output
        output.push_back(vec[top][vc.m_vecp[top]]);

        // advance winner's item stream
        vc.m_vecp[top]++;

        if (vc.m_vecp[top] == vec[top].size())
            wt.deactivate_player(top);
        else if (!test_rebuild)
            wt.replay_on_pop();

        if (test_rebuild)
            wt.rebuild();

        if (debug) std::cout << "after replay/rebuild: " << wt.to_string();
    }

    die_unless(output.size() == totalsize);

    if (debug)
    {
        for (size_t i = 0; i < output.size(); ++i)
            std::cout << output[i] << " ";
    }

    die_unless(output == correct);
}

int main()
{
    // run winner tree tests for 2..20 players
    for (size_t i = 2; i <= 20; ++i) {
        test_vecs(i, false);
        test_vecs(i, true);
    }

    return 0;
}
