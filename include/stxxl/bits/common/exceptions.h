/***************************************************************************
 *  include/stxxl/bits/common/exceptions.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_EXCEPTIONS_H_
#define STXXL_EXCEPTIONS_H_

#include <iostream>
#include <string>
#include <stdexcept>

#include "stxxl/bits/namespace.h"


__STXXL_BEGIN_NAMESPACE

class io_error : public std::ios_base::failure
{
public:
    io_error () throw () : std::ios_base::failure("") { }
    io_error (const std::string & msg_) throw () :
        std::ios_base::failure(msg_)
    { }
};

class resource_error : public std::runtime_error
{
public:
    resource_error () throw () : std::runtime_error("") { }
    resource_error (const std::string & msg_) throw () :
        std::runtime_error(msg_)
    { }
};

class bad_ext_alloc : public std::runtime_error
{
public:
    bad_ext_alloc () throw () : std::runtime_error("") { }
    bad_ext_alloc (const std::string & msg_) throw () :
        std::runtime_error(msg_)
    { }
};

__STXXL_END_NAMESPACE

#endif // !STXXL_EXCEPTIONS_H_
