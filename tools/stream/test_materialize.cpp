/***************************************************************************
 *  stream/test_materialize.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <vector>
#include <stxxl/stream>
#include <stxxl/vector>

struct forty_two
{
    unsigned counter;

    forty_two() : counter(0) { }

    bool empty() const { return !(counter < 42); }

    int operator * ()
    {
        assert(!empty());
        return 42;
    }

    forty_two & operator ++ ()
    {
        assert(!empty());
        ++counter;
        return *this;
    }

    forty_two & reset()
    {
        counter = 0;
        return *this;
    }
};

/*
    template <class OutputIterator_, class StreamAlgorithm_>
    OutputIterator_ materialize(StreamAlgorithm_ & in, OutputIterator_ out);

    template <class OutputIterator_, class StreamAlgorithm_>
    OutputIterator_ materialize(StreamAlgorithm_ & in, OutputIterator_ outbegin, OutputIterator_ outend);

    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_, class StreamAlgorithm_>
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>
    materialize(StreamAlgorithm_ & in,
                stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> outbegin,
                stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> outend,
                unsigned_type nbuffers = 0);

    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_, class StreamAlgorithm_>
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>
    materialize(StreamAlgorithm_ & in,
                stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> out,
                unsigned_type nbuffers = 0);
*/

int main()
{
    stxxl::config::get_instance();
    forty_two _42;
    std::vector<int> v(1000);
    stxxl::stream::materialize(_42.reset(), v.begin());
    stxxl::stream::materialize(_42.reset(), v.begin(), v.end());

    stxxl::VECTOR_GENERATOR<int>::result xv(1000);
    stxxl::stream::materialize(_42.reset(), xv.begin());
    stxxl::stream::materialize(_42.reset(), xv.begin(), 42);
    stxxl::stream::materialize(_42.reset(), xv.begin(), xv.end());
    stxxl::stream::materialize(_42.reset(), xv.begin(), xv.end(), 42);
}
