/***************************************************************************
 *  include/stxxl/bits/mng/block_alloc.h
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

#ifndef STXXL_MNG_BLOCK_ALLOC_HEADER
#define STXXL_MNG_BLOCK_ALLOC_HEADER

#include <stxxl/bits/parallel.h>
#include <stxxl/bits/mng/config.h>

#include <algorithm>
#include <random>
#include <vector>

namespace stxxl {

//! \defgroup alloc Allocation Functors
//! \ingroup mnglayer
//! Standard allocation strategies encapsulated in functors.
//! \{

//! Example disk allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct basic_allocation_strategy
{
    basic_allocation_strategy(int disks_begin, int disks_end);
    basic_allocation_strategy();
    int operator () (int i) const;
    static const char * name();
};

//! Striping disk allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct striping
{
    unsigned_type begin, diff;

public:
    striping(unsigned_type b, unsigned_type e) : begin(b), diff(e - b)
    { }

    striping() : begin(0)
    {
        diff = config::get_instance()->disks_number();
    }

    unsigned_type operator () (unsigned_type i) const
    {
        return begin + i % diff;
    }

    static const char * name()
    {
        return "striping";
    }
};

//! Fully randomized disk allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct fully_random : public striping
{
private:
    mutable std::default_random_engine rng_ { std::random_device { } () };

public:
    fully_random(unsigned_type b, unsigned_type e) : striping(b, e)
    { }

    fully_random() : striping()
    { }

    unsigned_type operator () (unsigned_type /*i*/) const
    {
        return begin + rng_() % diff;
    }

    static const char * name()
    {
        return "fully randomized striping";
    }
};

//! Simple randomized disk allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct simple_random : public striping
{
private:
    unsigned_type offset;

    void init()
    {
        std::default_random_engine rng { std::random_device { } () };
        offset = rng() % diff;
    }

public:
    simple_random(unsigned_type b, unsigned_type e) : striping(b, e)
    {
        init();
    }

    simple_random() : striping()
    {
        init();
    }

    unsigned_type operator () (unsigned_type i) const
    {
        return begin + (i + offset) % diff;
    }

    static const char * name()
    {
        return "simple randomized striping";
    }
};

//! Randomized cycling disk allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct random_cyclic : public striping
{
private:
    std::vector<unsigned_type> perm;

    void init()
    {
        for (unsigned_type i = 0; i < diff; i++)
            perm[i] = i;

        std::random_shuffle(perm.begin(), perm.end());
    }

public:
    random_cyclic(unsigned_type b, unsigned_type e) : striping(b, e), perm(diff)
    {
        init();
    }

    random_cyclic() : striping(), perm(diff)
    {
        init();
    }

    unsigned_type operator () (unsigned_type i) const
    {
        return begin + perm[i % diff];
    }

    static const char * name()
    {
        return "randomized cycling striping";
    }
};

struct random_cyclic_disk : public random_cyclic
{
    random_cyclic_disk(unsigned_type b, unsigned_type e) : random_cyclic(b, e)
    { }

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
    random_cyclic_flash(unsigned_type b, unsigned_type e) : random_cyclic(b, e)
    { }

    random_cyclic_flash()
        : random_cyclic(config::get_instance()->flash_range().first,
                        config::get_instance()->flash_range().second)
    { }

    static const char * name()
    {
        return "Randomized cycling striping on flash devices";
    }
};

//! 'Single disk' disk allocation scheme functor.
//! \remarks model of \b allocation_strategy concept
struct single_disk
{
    unsigned_type disk;

    explicit single_disk(unsigned_type d, unsigned_type = 0) : disk(d)
    { }

    single_disk() : disk(0)
    { }

    unsigned_type operator () (unsigned_type /*i*/) const
    {
        return disk;
    }

    static const char * name()
    {
        return "single disk";
    }
};

//! Allocator functor adaptor.
//!
//! Gives offset to disk number sequence defined in constructor
template <class BaseAllocator>
struct offset_allocator
{
    BaseAllocator base;
    int_type offset;

    //! Creates functor based on instance of \c BaseAllocator functor
    //! with offset \c offset_.
    //! \param offset_ offset
    //! \param base_ used to create a copy
    offset_allocator(int_type offset_, const BaseAllocator& base_) : base(base_), offset(offset_)
    { }

    //! Creates functor based on instance of \c BaseAllocator functor.
    //! \param base_ used to create a copy
    explicit offset_allocator(const BaseAllocator& base_) : base(base_), offset(0)
    { }

    //! Creates functor based on default \c BaseAllocator functor.
    offset_allocator() : offset(0)
    { }

    unsigned_type operator () (unsigned_type i) const
    {
        return base(offset + i);
    }

    int_type get_offset() const
    {
        return offset;
    }

    void set_offset(int_type i)
    {
        offset = i;
    }
};

#ifndef STXXL_DEFAULT_ALLOC_STRATEGY
    #define STXXL_DEFAULT_ALLOC_STRATEGY stxxl::random_cyclic
#endif

//! \}

} // namespace stxxl

#endif // !STXXL_MNG_BLOCK_ALLOC_HEADER
// vim: et:ts=4:sw=4
