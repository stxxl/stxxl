/***************************************************************************
 *  include/stxxl/bits/stream/unique.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM__UNIQUE_H
#define STXXL_STREAM__UNIQUE_H

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

//! \brief Stream package subnamespace
namespace stream
{
    ////////////////////////////////////////////////////////////////////////
    //     UNIQUE                                                         //
    ////////////////////////////////////////////////////////////////////////

    struct dummy_cmp_unique_ { };

    //! \brief Equivalent to std::unique algorithms
    //!
    //! Removes consecutive duplicates from the stream.
    //! Uses BinaryPredicate to compare elements of the stream
    template <class Input, class BinaryPredicate = dummy_cmp_unique_>
    class unique
    {
        Input & input;
        BinaryPredicate binary_pred;
        typename Input::value_type current;

    public:
        //! \brief Standard stream typedef
        typedef typename Input::value_type value_type;

        unique(Input & input_, BinaryPredicate binary_pred_) : input(input_), binary_pred(binary_pred_)
        {
            if (!input.empty())
                current = *input;
        }

        //! \brief Standard stream method
        unique & operator ++ ()
        {
            value_type old_value = current;
            ++input;
            while (!input.empty() && (binary_pred(current = *input, old_value)))
                ++input;
        }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        //! \brief Standard stream method
        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return input.empty();
        }
    };

    //! \brief Equivalent to std::unique algorithms
    //!
    //! Removes consecutive duplicates from the stream.
    template <class Input>
    class unique<Input, dummy_cmp_unique_>
    {
        Input & input;
        typename Input::value_type current;

    public:
        //! \brief Standard stream typedef
        typedef typename Input::value_type value_type;

        unique(Input & input_) : input(input_)
        {
            if (!input.empty())
                current = *input;
        }

        //! \brief Standard stream method
        unique & operator ++ ()
        {
            value_type old_value = current;
            ++input;
            while (!input.empty() && ((current = *input) == old_value))
                ++input;
        }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        //! \brief Standard stream method
        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return input.empty();
        }
    };

//! \}
}

__STXXL_END_NAMESPACE

#endif // !STXXL_STREAM__UNIQUE_H
// vim: et:ts=4:sw=4
