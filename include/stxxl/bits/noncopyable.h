#ifndef STXXL_NONCOPYABLE_HEADER
#define STXXL_NONCOPYABLE_HEADER

/***************************************************************************
 *            noncopyable.h
 *
 *  Thu Nov 22 16:44:07 CET 2007
 *  Copyright (C) 2007  Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *
 *  Inspired by boost::noncopyable.
 ****************************************************************************/

#ifdef STXXL_BOOST_CONFIG
#include <boost/noncopyable.hpp>
#endif

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

#ifdef STXXL_BOOST_CONFIG

typedef boost::noncopyable noncopyable;

#else

class noncopyable {
protected:
    noncopyable() { }

private:
    // copying and assignment is not allowed
    noncopyable(const noncopyable &);
    const noncopyable & operator = (const noncopyable &);
};

#endif

__STXXL_END_NAMESPACE

#endif // !STXXL_NONCOPYABLE_HEADER
