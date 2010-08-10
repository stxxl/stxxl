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
    template <class ValueType, class TriggerEntryType>
    struct sorted_runs
    {
        typedef ValueType value_type;  // may differ from trigger_entry_type::value_type
        typedef TriggerEntryType trigger_entry_type;
        typedef typename trigger_entry_type::block_type block_type;
        typedef std::vector<trigger_entry_type> run_type;
        typedef stxxl::external_size_type size_type;

        size_type elements;
        std::vector<run_type> runs;
        std::vector<unsigned_type> runs_sizes;

        // Optimization:
        // if the input is small such that its total size is
        // less than B (block_type::size)
        // then input is sorted internally
        // and kept in the array "small"
        std::vector<ValueType> small_;

        sorted_runs() : elements(0) { }

        //! \brief Deallocates the blocks which the runs occupy
        //!
        //! \remark Usually there is no need in calling this method,
        //! since the \c runs_merger calls it when it is being destructed
        void deallocate_blocks()
        {
            block_manager * bm = block_manager::get_instance();
            for (unsigned_type i = 0; i < runs.size(); ++i)
                bm->delete_blocks(
                    trigger_entry_iterator<typename run_type::iterator, block_type::raw_size>(runs[i].begin()),
                    trigger_entry_iterator<typename run_type::iterator, block_type::raw_size>(runs[i].end()));

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
