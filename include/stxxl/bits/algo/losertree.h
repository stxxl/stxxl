/***************************************************************************
 *  include/stxxl/bits/algo/losertree.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 1999 Peter Sanders <sanders@mpi-sb.mpg.de>
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_LOSERTREE_HEADER
#define STXXL_ALGO_LOSERTREE_HEADER

#include <algorithm>
#include <utility>

#include <foxxll/common/types.hpp>
#include <foxxll/common/utils.hpp>

#include <tlx/define.hpp>
#include <tlx/logger.hpp>

#include <stxxl/types>
#include <tlx/math/integer_log2.hpp>

namespace stxxl {

template <typename RunCursorType,
          typename RunCursorCmpType>
class loser_tree
{
    static constexpr bool debug = false;
    int logK;
    size_t k;
    size_t* entry;
    RunCursorType* current;
    RunCursorCmpType cmp;

    size_t init_winner(size_t root)
    {
        if (root >= k)
        {
            return root - k;
        }
        else
        {
            size_t left = init_winner(2 * root);
            size_t right = init_winner(2 * root + 1);
            if (cmp(current[left], current[right]))
            {
                entry[root] = right;
                return left;
            }
            else
            {
                entry[root] = left;
                return right;
            }
        }
    }

public:
    using prefetcher_type = typename RunCursorType::prefetcher_type;
    using value_type = typename RunCursorType::value_type;

    loser_tree(
        prefetcher_type* p,
        size_t nruns,
        RunCursorCmpType c)
        : cmp(c)
    {
        size_t i;
        logK = tlx::integer_log2_ceil(nruns);
        size_t kReg = k = size_t(1) << logK;

        LOG << "loser_tree: logK=" << logK << " nruns=" << nruns << " K=" << kReg;

#ifdef STXXL_SORT_SINGLE_PREFETCHER
        current = new RunCursorType[kReg];
        RunCursorType::set_prefetcher(p);
#else
        current = new RunCursorType[kReg];
        for (i = 0; i < kReg; ++i)
            current[i].prefetcher() = p;
#endif
        entry = new size_t[(kReg << 1)];
        // init cursors
        for (i = 0; i < nruns; ++i)
        {
            current[i].buffer = p->pull_block();
            //current[i].pos = 0; // done in constructor
            entry[kReg + i] = i;
        }

        for (i = nruns; i < kReg; ++i)
        {
            current[i].make_inf();
            entry[kReg + i] = i;
        }

        entry[0] = init_winner(1);
    }

    //! non-copyable: delete copy-constructor
    loser_tree(const loser_tree&) = delete;
    //! non-copyable: delete assignment operator
    loser_tree& operator = (const loser_tree&) = delete;

    ~loser_tree()
    {
        delete[] current;
        delete[] entry;
    }

    void swap(loser_tree& obj)
    {
        std::swap(logK, obj.logK);
        std::swap(k, obj.k);
        std::swap(entry, obj.entry);
        std::swap(current, obj.current);
        std::swap(cmp, obj.cmp);
    }

private:
    template <int LogK>
    void multi_merge_unrolled(value_type* out_first, value_type* out_last)
    {
        RunCursorType* currentE, * winnerE;
        size_t* regEntry = entry;
        size_t winnerIndex = regEntry[0];

        while (TLX_LIKELY(out_first != out_last))
        {
            winnerE = current + winnerIndex;
            *(out_first) = winnerE->current();
            ++out_first;

            ++(*winnerE);

#define TreeStep(L)                                                                                              \
    if (LogK >= L)                                                                                               \
    {                                                                                                            \
        currentE = current +                                                                                     \
                   regEntry[(winnerIndex + (1 << LogK)) >> (((int(LogK - L) + 1) >= 0) ? ((LogK - L) + 1) : 0)]; \
        if (cmp(*currentE, *winnerE))                                                                            \
        {                                                                                                        \
            std::swap(regEntry[(winnerIndex + (1 << LogK))                                                       \
                               >> (((int(LogK - L) + 1) >= 0) ? ((LogK - L) + 1) : 0)], winnerIndex);            \
            winnerE = currentE;                                                                                  \
        }                                                                                                        \
    }

            TreeStep(10);
            TreeStep(9);
            TreeStep(8);
            TreeStep(7);
            TreeStep(6);
            TreeStep(5);
            TreeStep(4);
            TreeStep(3);
            TreeStep(2);
            TreeStep(1);

#undef TreeStep
        }

        regEntry[0] = winnerIndex;
    }

    void multi_merge_unrolled_0(value_type* out_first, value_type* out_last)
    {
        while (TLX_LIKELY(out_first != out_last))
        {
            *out_first = current->current();
            ++out_first;
            ++(*current);
        }
    }

    void multi_merge_k(value_type* out_first, value_type* out_last)
    {
        RunCursorType* currentE, * winnerE;
        size_t kReg = k;
        size_t winnerIndex = entry[0];

        while (TLX_LIKELY(out_first != out_last))
        {
            winnerE = current + winnerIndex;
            *(out_first) = winnerE->current();
            ++out_first;

            ++(*winnerE);

            for (size_t i = (winnerIndex + kReg) >> 1; i > 0; i >>= 1)
            {
                currentE = current + entry[i];

                if (cmp(*currentE, *winnerE))
                {
                    std::swap(entry[i], winnerIndex);
                    winnerE = currentE;
                }
            }
        }

        entry[0] = winnerIndex;
    }

public:
    void multi_merge(value_type* out_first, value_type* out_last)
    {
        switch (logK)
        {
        case 0:
            multi_merge_unrolled_0(out_first, out_last);
            break;
        case 1:
            multi_merge_unrolled<1>(out_first, out_last);
            break;
        case 2:
            multi_merge_unrolled<2>(out_first, out_last);
            break;
        case 3:
            multi_merge_unrolled<3>(out_first, out_last);
            break;
        case 4:
            multi_merge_unrolled<4>(out_first, out_last);
            break;
        case 5:
            multi_merge_unrolled<5>(out_first, out_last);
            break;
        case 6:
            multi_merge_unrolled<6>(out_first, out_last);
            break;
        case 7:
            multi_merge_unrolled<7>(out_first, out_last);
            break;
        case 8:
            multi_merge_unrolled<8>(out_first, out_last);
            break;
        case 9:
            multi_merge_unrolled<9>(out_first, out_last);
            break;
        case 10:
            multi_merge_unrolled<10>(out_first, out_last);
            break;
        default:
            multi_merge_k(out_first, out_last);
            break;
        }
    }
};

} // namespace stxxl

namespace std {

template <typename RunCursorType,
          typename RunCursorCmpType>
void swap(stxxl::loser_tree<RunCursorType, RunCursorCmpType>& a,
          stxxl::loser_tree<RunCursorType, RunCursorCmpType>& b)
{
    a.swap(b);
}

} // namespace std

#endif // !STXXL_ALGO_LOSERTREE_HEADER
// vim: et:ts=4:sw=4
