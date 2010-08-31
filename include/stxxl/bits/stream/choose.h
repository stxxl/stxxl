/***************************************************************************
 *  include/stxxl/bits/stream/choose.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM__CHOOSE_H_
#define STXXL_STREAM__CHOOSE_H_

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

//! \brief Stream package subnamespace
namespace stream
{
    ////////////////////////////////////////////////////////////////////////
    //     CHOOSE                                                         //
    ////////////////////////////////////////////////////////////////////////

    template <class Input_, int Which>
    class choose
    { };

    //! \brief Creates stream from a tuple stream taking the first component of each tuple
    //!
    //! \tparam Input_ type of the input tuple stream
    //!
    //! \remark Tuple stream is a stream which \c value_type is \c stxxl::tuple .
    template <class Input_>
    class choose<Input_, 1>
    {
        Input_ & in;

        typedef typename Input_::value_type tuple_type;

    public:
        //! \brief Standard stream typedef
        typedef typename tuple_type::first_type value_type;

        //! \brief Construction
        choose(Input_ & in_) : in(in_)
        { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return (*in).first;
        }

        const value_type * operator -> () const
        {
            return &(*in).first;
        }

        //! \brief Standard stream method
        choose & operator ++ ()
        {
            ++in;
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return in.empty();
        }
    };

    //! \brief Creates stream from a tuple stream taking the second component of each tuple
    //!
    //! \tparam Input_ type of the input tuple stream
    //!
    //! \remark Tuple stream is a stream which \c value_type is \c stxxl::tuple .
    template <class Input_>
    class choose<Input_, 2>
    {
        Input_ & in;

        typedef typename Input_::value_type tuple_type;

    public:
        //! \brief Standard stream typedef
        typedef typename tuple_type::second_type value_type;

        //! \brief Construction
        choose(Input_ & in_) : in(in_)
        { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return (*in).second;
        }

        const value_type * operator -> () const
        {
            return &(*in).second;
        }

        //! \brief Standard stream method
        choose & operator ++ ()
        {
            ++in;
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return in.empty();
        }
    };

    //! \brief Creates stream from a tuple stream taking the third component of each tuple
    //!
    //! \tparam Input_ type of the input tuple stream
    //!
    //! \remark Tuple stream is a stream which \c value_type is \c stxxl::tuple .
    template <class Input_>
    class choose<Input_, 3>
    {
        Input_ & in;

        typedef typename Input_::value_type tuple_type;

    public:
        //! \brief Standard stream typedef
        typedef typename tuple_type::third_type value_type;

        //! \brief Construction
        choose(Input_ & in_) : in(in_)
        { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return (*in).third;
        }

        const value_type * operator -> () const
        {
            return &(*in).third;
        }

        //! \brief Standard stream method
        choose & operator ++ ()
        {
            ++in;
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return in.empty();
        }
    };

    //! \brief Creates stream from a tuple stream taking the fourth component of each tuple
    //!
    //! \tparam Input_ type of the input tuple stream
    //!
    //! \remark Tuple stream is a stream which \c value_type is \c stxxl::tuple .
    template <class Input_>
    class choose<Input_, 4>
    {
        Input_ & in;

        typedef typename Input_::value_type tuple_type;

    public:
        //! \brief Standard stream typedef
        typedef typename tuple_type::fourth_type value_type;

        //! \brief Construction
        choose(Input_ & in_) : in(in_)
        { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return (*in).fourth;
        }

        const value_type * operator -> () const
        {
            return &(*in).fourth;
        }

        //! \brief Standard stream method
        choose & operator ++ ()
        {
            ++in;
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return in.empty();
        }
    };

    //! \brief Creates stream from a tuple stream taking the fifth component of each tuple
    //!
    //! \tparam Input_ type of the input tuple stream
    //!
    //! \remark Tuple stream is a stream which \c value_type is \c stxxl::tuple .
    template <class Input_>
    class choose<Input_, 5>
    {
        Input_ & in;

        typedef typename Input_::value_type tuple_type;

    public:
        //! \brief Standard stream typedef
        typedef typename tuple_type::fifth_type value_type;

        //! \brief Construction
        choose(Input_ & in_) : in(in_)
        { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return (*in).fifth;
        }

        const value_type * operator -> () const
        {
            return &(*in).fifth;
        }

        //! \brief Standard stream method
        choose & operator ++ ()
        {
            ++in;
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return in.empty();
        }
    };

    //! \brief Creates stream from a tuple stream taking the sixth component of each tuple
    //!
    //! \tparam Input_ type of the input tuple stream
    //!
    //! \remark Tuple stream is a stream which \c value_type is \c stxxl::tuple .
    template <class Input_>
    class choose<Input_, 6>
    {
        Input_ & in;

        typedef typename Input_::value_type tuple_type;

    public:
        //! \brief Standard stream typedef
        typedef typename tuple_type::sixth_type value_type;

        //! \brief Construction
        choose(Input_ & in_) : in(in_)
        { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return (*in).sixth;
        }

        const value_type * operator -> () const
        {
            return &(*in).sixth;
        }

        //! \brief Standard stream method
        choose & operator ++ ()
        {
            ++in;
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return in.empty();
        }
    };


//! \}
}

__STXXL_END_NAMESPACE


#include <stxxl/bits/stream/unique.h>


#endif // !STXXL_STREAM__CHOOSE_H_
// vim: et:ts=4:sw=4
