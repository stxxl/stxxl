/***************************************************************************
 *  include/stxxl/bits/mng/block_alloc_strategy.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_BLOCK_ALLOC_STRATEGY_HEADER
#define STXXL_MNG_BLOCK_ALLOC_STRATEGY_HEADER

#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/bits/mng/config.h>

#include <algorithm>
#include <random>
#include <vector>

namespace stxxl {

//! \defgroup alloc Allocation Functors
//! \ingroup mnglayer
//! Standard allocation strategies encapsulated in functors.
//! \{

//! Example parallel disk block allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct basic_allocation_strategy
{
    basic_allocation_strategy(size_t disks_begin, size_t disks_end);
    basic_allocation_strategy();
    size_t operator () (size_t i) const;
    static const char * name();
};

//! Striping parallel disk block allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct striping
{
    size_t begin_, diff_;

public:
    striping(size_t begin, size_t end)
        : begin_(begin), diff_(end - begin) { }

    striping() : begin_(0)
    {
        diff_ = config::get_instance()->disks_number();
    }

    size_t operator () (size_t i) const
    {
        return begin_ + i % diff_;
    }

    static const char * name()
    {
        return "striping";
    }
};

//! Fully randomized parallel disk block allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct fully_random : public striping
{
private:
    mutable std::default_random_engine rng_ { std::random_device { } () };

public:
    fully_random(size_t begin, size_t end) : striping(begin, end) { }

    fully_random() : striping() { }

    size_t operator () (size_t /* i */) const
    {
        return begin_ + rng_() % diff_;
    }

    static const char * name()
    {
        return "fully randomized striping";
    }
};

//! Simple randomized parallel disk block allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct simple_random : public striping
{
private:
    size_t offset;

    void init()
    {
        std::default_random_engine rng { std::random_device { } () };
        offset = rng() % diff_;
    }

public:
    simple_random(size_t begin, size_t end) : striping(begin, end)
    {
        init();
    }

    simple_random() : striping()
    {
        init();
    }

    size_t operator () (size_t i) const
    {
        return begin_ + (i + offset) % diff_;
    }

    static const char * name()
    {
        return "simple randomized striping";
    }
};

//! Randomized cycling parallel disk block allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct random_cyclic : public striping
{
private:
    std::vector<size_t> perm_;

    void init()
    {
        for (size_t i = 0; i < diff_; i++)
            perm_[i] = i;

        std::random_shuffle(perm_.begin(), perm_.end());
    }

public:
    random_cyclic(size_t begin, size_t end)
        : striping(begin, end), perm_(diff_)
    {
        init();
    }

    random_cyclic() : striping(), perm_(diff_)
    {
        init();
    }

    size_t operator () (size_t i) const
    {
        return begin_ + perm_[i % diff_];
    }

    static const char * name()
    {
        return "randomized cycling striping";
    }
};

struct random_cyclic_disk : public random_cyclic
{
    random_cyclic_disk(size_t begin, size_t end) : random_cyclic(begin, end) { }

    random_cyclic_disk()
        : random_cyclic(config::get_instance()->regular_disk_range().first,
                        config::get_instance()->regular_disk_range().second)
    { }

    static const char * name()
    {
        return "Randomized cycling striping on regular disks";
    }
};

struct random_cyclic_flash : public random_cyclic
{
    random_cyclic_flash(size_t begin, size_t end)
        : random_cyclic(begin, end) { }

    random_cyclic_flash()
        : random_cyclic(config::get_instance()->flash_range().first,
                        config::get_instance()->flash_range().second)
    { }

    static const char * name()
    {
        return "Randomized cycling striping on flash devices";
    }
};

//! 'Single disk' parallel disk block allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct single_disk
{
    size_t disk_;

    explicit single_disk(size_t disk = 0, size_t = 0) : disk_(disk) { }

    size_t operator () (size_t /* i */) const
    {
        return disk_;
    }

    static const char * name()
    {
        return "single disk";
    }
};

//! Allocator functor adapter.
//!
//! Gives offset to disk number sequence defined in constructor
template <class BaseAllocator>
struct offset_allocator
{
    BaseAllocator base_;
    int offset_;

    //! Creates functor based on instance of \c BaseAllocator functor
    //! with offset.
    //! \param offset offset
    //! \param base_ used to create a copy
    offset_allocator(int offset, const BaseAllocator& base)
        : base_(base), offset_(offset) { }

    //! Creates functor based on instance of \c BaseAllocator functor.
    //! \param base_ used to create a copy
    explicit offset_allocator(const BaseAllocator& base)
        : base_(base), offset_(0) { }

    //! Creates functor based on default \c BaseAllocator functor.
    offset_allocator() : offset_(0) { }

    size_t operator () (size_t i) const
    {
        return base_(offset_ + i);
    }

    int get_offset() const
    {
        return offset_;
    }

    void set_offset(int i)
    {
        offset_ = i;
    }
};

#ifndef STXXL_DEFAULT_ALLOC_STRATEGY
    #define STXXL_DEFAULT_ALLOC_STRATEGY stxxl::random_cyclic
#endif

//! \}

} // namespace stxxl

#endif // !STXXL_MNG_BLOCK_ALLOC_STRATEGY_HEADER
// vim: et:ts=4:sw=4
