/***************************************************************************
 *  include/stxxl/bits/stream/sorted_runs.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM_SORTED_RUNS_HEADER
#define STXXL_STREAM_SORTED_RUNS_HEADER

#include <algorithm>
#include <utility>
#include <vector>

#include <tlx/counting_ptr.hpp>

#include <foxxll/mng/block_manager.hpp>
#include <foxxll/mng/typed_block.hpp>

#include <stxxl/bits/algo/trigger_entry.h>

namespace stxxl {
namespace stream {

//! \addtogroup streampack Stream Package
//! \{

////////////////////////////////////////////////////////////////////////
//     SORTED RUNS                                                    //
////////////////////////////////////////////////////////////////////////

//! All sorted runs of a sort operation.
template <typename TriggerEntryType, typename CompareType>
struct sorted_runs : public tlx::reference_counter
{
    using trigger_entry_type = TriggerEntryType;
    using block_type = typename trigger_entry_type::block_type;
    //! may differ from trigger_entry_type::value_type
    using value_type = typename block_type::value_type;
    using run_type = std::vector<trigger_entry_type>;
    using small_run_type = std::vector<value_type>;
    using size_type = stxxl::external_size_type;
    using run_index_type = typename std::vector<run_type>::size_type;

    using cmp_type = CompareType;

    //! total number of elements in all runs
    size_type elements;

    //! vector of runs (containing vectors of block ids)
    std::vector<run_type> runs;

    //! vector of the number of elements in each individual run
    std::vector<size_type> runs_sizes;

    //! Small sort optimization:
    // if the input is small such that its total size is at most B
    // (block_type::size) then input is sorted internally and kept in the
    // array "small_run"
    small_run_type small_run;

public:
    sorted_runs()
        : elements(0)
    { }

    //! non-copyable: delete copy-constructor
    sorted_runs(const sorted_runs&) = delete;
    //! non-copyable: delete assignment operator
    sorted_runs& operator = (const sorted_runs&) = delete;

    ~sorted_runs()
    {
        deallocate_blocks();
    }

    //! Clear the internal state of the object: release all runs and reset.
    void clear()
    {
        deallocate_blocks();

        elements = 0;
        runs.clear();
        runs_sizes.clear();
        small_run.clear();
    }

    //! Add a new run with given number of elements
    void add_run(const run_type& run, size_type run_size)
    {
        runs.push_back(run);
        runs_sizes.push_back(run_size);
        elements += run_size;
    }

    //! Swap contents with another object. This is used by the recursive
    //! merger to swap in a sorted_runs object with fewer runs.
    void swap(sorted_runs& b)
    {
        std::swap(elements, b.elements);
        std::swap(runs, b.runs);
        std::swap(runs_sizes, b.runs_sizes);
        std::swap(small_run, b.small_run);
    }

private:
    //! Deallocates the blocks which the runs occupy.
    //!
    //! \remark There is no need in calling this method, the blocks are
    //! deallocated by the destructor. However, if you wish to reuse the
    //! object, then this function can be used to clear its state.
    void deallocate_blocks()
    {
        foxxll::block_manager* bm = foxxll::block_manager::get_instance();
        for (size_t i = 0; i < runs.size(); ++i)
        {
            bm->delete_blocks(make_bid_iterator(runs[i].begin()),
                              make_bid_iterator(runs[i].end()));
        }
    }
};

//! \}

} // namespace stream
} // namespace stxxl

#endif // !STXXL_STREAM_SORTED_RUNS_HEADER
// vim: et:ts=4:sw=4
