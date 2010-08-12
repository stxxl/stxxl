/***************************************************************************
 *  include/stxxl/bits/stream/sorted_runs.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM__SORTED_RUNS_H
#define STXXL_STREAM__SORTED_RUNS_H

#include <vector>
#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/algo/adaptor.h>


__STXXL_BEGIN_NAMESPACE

namespace stream
{
    //! \addtogroup streampack Stream package
    //! \{


    ////////////////////////////////////////////////////////////////////////
    //     SORTED RUNS                                                    //
    ////////////////////////////////////////////////////////////////////////

    //! \brief All sorted runs of a sort operation.
    template <typename TriggerEntryType>
    struct sorted_runs
    {
        typedef TriggerEntryType trigger_entry_type;
        typedef typename trigger_entry_type::block_type block_type;
        typedef typename block_type::value_type value_type;  // may differ from trigger_entry_type::value_type
        typedef std::vector<trigger_entry_type> run_type;
        typedef std::vector<value_type> small_run_type;
        typedef stxxl::external_size_type size_type;
        typedef typename std::vector<run_type>::size_type run_index_type;

        size_type elements;
        std::vector<run_type> runs;
        std::vector<size_type> runs_sizes;

        // Optimization:
        // if the input is small such that its total size is
        // at most B (block_type::size)
        // then input is sorted internally
        // and kept in the array "small"
        small_run_type small_;

        sorted_runs() : elements(0) { }

        //! \brief Adds a small (at most B elements) sorted run.
        //!
        //! \param first, last: input iterator pair of value_type
        template <typename InputIterator>
        void add_small_run(const InputIterator & first, const InputIterator & last)
        {
            assert(runs.empty());
            assert(small_.empty());
            small_.insert(small_.end(), first, last);
            elements += small_.size();
        }

        const small_run_type & small_run() const
        {
            return small_;
        }

        //! \brief Adds a sorted run.
        //!
        //! \param first, last: input iterator pair of trigger_entry_type
        //! \param length: number of elements (the last block is not neccessarily full)
        template <typename InputIterator>
        void add_run(const InputIterator & first, const InputIterator & last, size_type length)
        {
            assert(small_.empty());
            runs.push_back(run_type());
            runs.back().insert(runs.back().end(), first, last);
            runs_sizes.push_back(length);
            elements += length;
        }

        //! \brief Deallocates the blocks which the runs occupy
        //!
        //! \remark Usually there is no need in calling this method,
        //! since the \c runs_merger calls it when it is being destructed
        void deallocate_blocks()
        {
            block_manager * bm = block_manager::get_instance();
            for (unsigned_type i = 0; i < runs.size(); ++i)
                bm->delete_blocks(make_bid_iterator(runs[i].begin()), make_bid_iterator(runs[i].end()));

            runs.clear();
        }

        // returns number of elements in all runs together
        size_type size() const
        {
            return elements;
        }
    };


//! \}
}

__STXXL_END_NAMESPACE

#endif // !STXXL_STREAM__SORTED_RUNS_H
// vim: et:ts=4:sw=4
