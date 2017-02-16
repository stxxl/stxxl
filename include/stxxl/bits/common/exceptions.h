/***************************************************************************
 *  include/stxxl/bits/common/exceptions.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_EXCEPTIONS_HEADER
#define STXXL_COMMON_EXCEPTIONS_HEADER

#include <iostream>
#include <stdexcept>
#include <string>

namespace stxxl {

class io_error : public std::ios_base::failure
{
public:
    io_error() noexcept
        : std::ios_base::failure("")
    { }

    explicit io_error(const std::string& message) noexcept
        : std::ios_base::failure(message)
    { }
};

class resource_error : public std::runtime_error
{
public:
    resource_error() noexcept
        : std::runtime_error("")
    { }

    explicit resource_error(const std::string& message) noexcept
        : std::runtime_error(message)
    { }
};

class bad_ext_alloc : public std::runtime_error
{
public:
    bad_ext_alloc() noexcept
        : std::runtime_error("")
    { }

    explicit bad_ext_alloc(const std::string& message) noexcept
        : std::runtime_error(message)
    { }
};

class bad_parameter : public std::runtime_error
{
public:
    bad_parameter() noexcept
        : std::runtime_error("")
    { }

    explicit bad_parameter(const std::string& message) noexcept
        : std::runtime_error(message)
    { }
};

class unreachable : public std::runtime_error
{
public:
    unreachable() noexcept
        : std::runtime_error("")
    { }

    explicit unreachable(const std::string& message) noexcept
        : std::runtime_error(message)
    { }
};

} // namespace stxxl

#endif // !STXXL_COMMON_EXCEPTIONS_HEADER
// vim: et:ts=4:sw=4
