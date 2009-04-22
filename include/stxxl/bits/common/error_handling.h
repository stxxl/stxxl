/***************************************************************************
 *  include/stxxl/bits/common/error_handling.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ERROR_HANDLING_HEADER
#define STXXL_ERROR_HANDLING_HEADER

#include <sstream>
#include <cerrno>
#include <cstring>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/exceptions.h>


__STXXL_BEGIN_NAMESPACE

#define __STXXL_STRING(x) # x

#ifdef BOOST_MSVC
 #define STXXL_PRETTY_FUNCTION_NAME __FUNCTION__
#else
 #define STXXL_PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

////////////////////////////////////////////////////////////////////////////

#define STXXL_THROW(exception_type, location, error_message) \
    { \
        std::ostringstream msg_; \
        msg_ << "Error in " << location << ": " << error_message; \
        throw exception_type(msg_.str()); \
    }

#define STXXL_THROW2(exception_type, error_message) \
    STXXL_THROW(exception_type, "function " << STXXL_PRETTY_FUNCTION_NAME, \
                "Info: " << error_message << " " << strerror(errno))

#define STXXL_THROW_INVALID_ARGUMENT(error_message) \
    STXXL_THROW(std::invalid_argument, \
                "function " << STXXL_PRETTY_FUNCTION_NAME, \
                error_message)

////////////////////////////////////////////////////////////////////////////

template <typename E>
inline void stxxl_util_function_error(const char * func_name, const char * expr = 0, const char * error = 0)
{
    std::ostringstream str_;
    str_ << "Error in function " << func_name << " " << (expr ? expr : strerror(errno));
    if (error)
        str_ << " " << error;
    throw E(str_.str());
}

#define stxxl_function_error(exception_type) \
    stxxl::stxxl_util_function_error<exception_type>(STXXL_PRETTY_FUNCTION_NAME)

template <typename E>
inline bool helper_check_success(bool success, const char * func_name, const char * expr = 0, const char * error = 0)
{
    if (!success)
        stxxl_util_function_error<E>(func_name, expr, error);
    return success;
}

template <typename E, typename INT>
inline bool helper_check_eq_0(INT res, const char * func_name, const char * expr, bool res_2_strerror = false)
{
    return helper_check_success<E>(res == 0, func_name, expr, res_2_strerror ? strerror(res) : 0);
}

#define check_pthread_call(expr) \
    stxxl::helper_check_eq_0<stxxl::resource_error>(expr, STXXL_PRETTY_FUNCTION_NAME, __STXXL_STRING(expr), true)

template <typename E, typename INT>
inline bool helper_check_ge_0(INT res, const char * func_name)
{
    return helper_check_success<E>(res >= 0, func_name);
}

#define stxxl_check_ge_0(expr, exception_type) \
    stxxl::helper_check_ge_0<exception_type>(expr, STXXL_PRETTY_FUNCTION_NAME)

template <typename E, typename INT>
inline bool helper_check_ne_0(INT res, const char * func_name)
{
    return helper_check_success<E>(res != 0, func_name);
}

#define stxxl_check_ne_0(expr, exception_type) \
    stxxl::helper_check_ne_0<exception_type>(expr, STXXL_PRETTY_FUNCTION_NAME)

__STXXL_END_NAMESPACE

#endif // !STXXL_ERROR_HANDLING_HEADER
