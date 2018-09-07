/***************************************************************************
 *  include/stxxl/bits/stream/stream.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2018 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM_STREAM_HEADER
#define STXXL_STREAM_STREAM_HEADER

#include <cassert>
#include <memory>

#include <tlx/define.hpp>
#include <tlx/meta/apply_tuple.hpp>
#include <tlx/meta/call_foreach_tuple.hpp>
#include <tlx/meta/fold_left_tuple.hpp>
#include <tlx/meta/vmap_foreach_tuple.hpp>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/mng/buf_istream.hpp>
#include <foxxll/mng/buf_ostream.hpp>

#include <stxxl/vector>

namespace stxxl {

//! Stream package subnamespace.
namespace stream {

//! \addtogroup streampack
//! \{

////////////////////////////////////////////////////////////////////////
//     STREAMIFY                                                      //
////////////////////////////////////////////////////////////////////////

//! A model of stream that retrieves the data from an input iterator.
//! For convenience use \c streamify function instead of direct instantiation
//! of \c iterator2stream .
template <typename InputIterator>
class iterator2stream
{
    InputIterator m_current, m_end;

public:
    //! Standard stream typedef.
    using value_type = typename std::iterator_traits<InputIterator>::value_type;

    iterator2stream(InputIterator begin, InputIterator end)
        : m_current(begin), m_end(end)
    { }

    iterator2stream(const iterator2stream& a)
        : m_current(a.m_current), m_end(a.m_end)
    { }

    //! Standard stream method.
    const value_type& operator * () const
    {
        return *m_current;
    }

    const value_type* operator -> () const
    {
        return &(*m_current);
    }

    //! Standard stream method.
    iterator2stream& operator ++ ()
    {
        assert(m_end != m_current);
        ++m_current;
        return *this;
    }

    //! Standard stream method.
    bool empty() const
    {
        return (m_current == m_end);
    }
};

//! Input iterator range to stream converter.
//! \param first iterator, pointing to the first value
//! \param last iterator, pointing to the last + 1 position, i.e. beyond the range
//! \return an instance of a stream object
template <typename InputIterator>
auto streamify(InputIterator first, InputIterator last)
{
    return iterator2stream<InputIterator>(first, last);
}

//! Traits class of \c streamify function.
template <typename InputIterator>
struct streamify_traits
{
    //! return type (stream type) of \c streamify for \c InputIterator.
    using stream_type = iterator2stream<InputIterator>;
};

//! A model of stream that retrieves data from an external \c stxxl::vector
//! iterator.  It is more efficient than generic \c iterator2stream thanks to
//! use of overlapping For convenience use \c streamify function instead of
//! direct instantiation of \c vector_iterator2stream .
template <typename InputIterator>
class vector_iterator2stream
{
    InputIterator m_current, m_end;
    using buf_istream_type = foxxll::buf_istream<typename InputIterator::block_type,
                                                 typename InputIterator::bids_container_iterator>;

    using buf_istream_unique_ptr_type = std::unique_ptr<buf_istream_type>;
    mutable buf_istream_unique_ptr_type in;

    void delete_stream()
    {
        in.reset();      // delete object
    }

public:
    //! Standard stream typedef.
    using value_type = typename std::iterator_traits<InputIterator>::value_type;

    vector_iterator2stream(InputIterator begin, InputIterator end,
                           size_t nbuffers = 0)
        : m_current(begin), m_end(end),
          in(static_cast<buf_istream_type*>(nullptr))
    {
        if (empty())
            return;

        begin.flush();         // flush container
        typename InputIterator::bids_container_iterator end_iter
            = end.bid() + ((end.block_offset()) ? 1 : 0);

        if (end_iter - begin.bid() > 0)
        {
            in.reset(new buf_istream_type(
                         begin.bid(), end_iter, nbuffers ? nbuffers :
                         (2 * foxxll::config::get_instance()->disks_number())));

            InputIterator cur = begin - begin.block_offset();

            // skip the beginning of the block
            for ( ; cur != begin; ++cur)
                ++(*in);
        }
    }

    //! non-copyable: delete copy-constructor
    vector_iterator2stream(const vector_iterator2stream&) = delete;
    //! non-copyable: delete assignment operator
    vector_iterator2stream& operator = (const vector_iterator2stream&) = delete;
    //! move-constructor: default
    vector_iterator2stream(vector_iterator2stream&&) = default;
    //! move-assignment operator: default
    vector_iterator2stream& operator = (vector_iterator2stream&&) = default;

    //! Standard stream method.
    const value_type& operator * () const
    {
        return **in;
    }

    const value_type* operator -> () const
    {
        return &(**in);
    }

    //! Standard stream method.
    vector_iterator2stream& operator ++ ()
    {
        assert(m_end != m_current);
        ++m_current;
        ++(*in);
        if (TLX_UNLIKELY(empty()))
            delete_stream();

        return *this;
    }

    //! Standard stream method.
    bool empty() const
    {
        return (m_current == m_end);
    }

    virtual ~vector_iterator2stream()
    {
        delete_stream();          // not needed actually
    }
};

//! Input external \c stxxl::vector iterator range to stream converter.
//! It is more efficient than generic input iterator \c streamify thanks to use of overlapping
//! \param first iterator, pointing to the first value
//! \param last iterator, pointing to the last + 1 position, i.e. beyond the range
//! \param nbuffers number of blocks used for overlapped reading (0 is default,
//! which equals to (2 * number_of_disks)
//! \return an instance of a stream object

template <typename VectorConfig>
auto streamify(
    stxxl::vector_iterator<VectorConfig> first,
    stxxl::vector_iterator<VectorConfig> last,
    size_t nbuffers = 0)
{
    TLX_LOG0 << "streamify for vector_iterator range is called";
    return vector_iterator2stream<stxxl::vector_iterator<VectorConfig> >(
        first, last, nbuffers);
}

template <typename VectorConfig>
struct streamify_traits<stxxl::vector_iterator<VectorConfig> >
{
    using stream_type = vector_iterator2stream<stxxl::vector_iterator<VectorConfig> >;
};

//! Input external \c stxxl::vector const iterator range to stream converter.
//! It is more efficient than generic input iterator \c streamify thanks to use of overlapping
//! \param first const iterator, pointing to the first value
//! \param last const iterator, pointing to the last + 1 position, i.e. beyond the range
//! \param nbuffers number of blocks used for overlapped reading (0 is default,
//! which equals to (2 * number_of_disks)
//! \return an instance of a stream object

template <typename VectorConfig>
auto streamify(
    stxxl::const_vector_iterator<VectorConfig> first,
    stxxl::const_vector_iterator<VectorConfig> last,
    size_t nbuffers = 0)
{
    TLX_LOG0 << "streamify for const_vector_iterator range is called";
    return vector_iterator2stream<stxxl::const_vector_iterator<VectorConfig> >(
        first, last, nbuffers);
}

template <typename VectorConfig>
struct streamify_traits<stxxl::const_vector_iterator<VectorConfig> >
{
    using stream_type = vector_iterator2stream<stxxl::const_vector_iterator<VectorConfig> >;
};

//! Version of  \c iterator2stream. Switches between \c vector_iterator2stream and \c iterator2stream .
//!
//! small range switches between
//! \c vector_iterator2stream and \c iterator2stream .
//! iterator2stream is chosen if the input iterator range
//! is small ( < B )
template <typename InputIterator>
class vector_iterator2stream_sr
{
    static constexpr bool debug = false;

    vector_iterator2stream<InputIterator>* vec_it_stream;
    iterator2stream<InputIterator>* it_stream;

    using block_type = typename InputIterator::block_type;

public:
    //! Standard stream typedef.
    using value_type = typename std::iterator_traits<InputIterator>::value_type;

    vector_iterator2stream_sr(InputIterator begin, InputIterator end,
                              size_t nbuffers = 0)
    {
        if (end - begin < block_type::size)
        {
            TLX_LOG << "vector_iterator2stream_sr::vector_iterator2stream_sr: Choosing iterator2stream<InputIterator>";
            it_stream = new iterator2stream<InputIterator>(begin, end);
            vec_it_stream = nullptr;
        }
        else
        {
            TLX_LOG << "vector_iterator2stream_sr::vector_iterator2stream_sr: Choosing vector_iterator2stream<InputIterator>";
            it_stream = nullptr;
            vec_it_stream = new vector_iterator2stream<InputIterator>(begin, end, nbuffers);
        }
    }

    //! Standard stream method.
    const value_type& operator * () const
    {
        if (it_stream)
            return **it_stream;

        return **vec_it_stream;
    }

    const value_type* operator -> () const
    {
        if (it_stream)
            return &(**it_stream);

        return &(**vec_it_stream);
    }

    //! Standard stream method.
    vector_iterator2stream_sr& operator ++ ()
    {
        if (it_stream)
            ++(*it_stream);

        else
            ++(*vec_it_stream);

        return *this;
    }

    //! Standard stream method.
    bool empty() const
    {
        if (it_stream)
            return it_stream->empty();

        return vec_it_stream->empty();
    }

    virtual ~vector_iterator2stream_sr()
    {
        if (it_stream)
            delete it_stream;

        else
            delete vec_it_stream;
    }
};

//! Version of \c streamify. Switches from \c vector_iterator2stream to \c
//! iterator2stream for small ranges.
template <typename VectorConfig>
auto streamify_sr(
    stxxl::vector_iterator<VectorConfig> first,
    stxxl::vector_iterator<VectorConfig> last,
    size_t nbuffers = 0)
{
    TLX_LOG0 << "streamify_sr for vector_iterator range is called";
    return vector_iterator2stream_sr<stxxl::vector_iterator<VectorConfig> >(
        first, last, nbuffers);
}

//! Version of \c streamify. Switches from \c vector_iterator2stream to \c
//! iterator2stream for small ranges.
template <typename VectorConfig>
auto streamify_sr(
    stxxl::const_vector_iterator<VectorConfig> first,
    stxxl::const_vector_iterator<VectorConfig> last,
    size_t nbuffers = 0)
{
    TLX_LOG0 << "streamify_sr for const_vector_iterator range is called";
    return vector_iterator2stream_sr<
        stxxl::const_vector_iterator<VectorConfig> >(first, last, nbuffers);
}

////////////////////////////////////////////////////////////////////////
//     GENERATE                                                       //
////////////////////////////////////////////////////////////////////////

//! A model of stream that outputs data from an adaptable generator functor.
//! For convenience use \c streamify function instead of direct instantiation
//! of \c generator2stream .
template <typename Generator, typename T = typename Generator::value_type>
class generator2stream
{
public:
    //! Standard stream typedef.
    using value_type = T;

private:
    Generator gen_;
    value_type m_current;

public:
    explicit generator2stream(Generator g)
        : gen_(g), m_current(gen_())
    { }

    generator2stream(const generator2stream& a) : gen_(a.gen_), m_current(a.m_current) { }

    //! Standard stream method.
    const value_type& operator * () const
    {
        return m_current;
    }

    const value_type* operator -> () const
    {
        return &m_current;
    }

    //! Standard stream method.
    generator2stream& operator ++ ()
    {
        m_current = gen_();
        return *this;
    }

    //! Standard stream method.
    bool empty() const
    {
        return false;
    }
};

//! Adaptable generator to stream converter.
//! \param gen_ generator object
//! \return an instance of a stream object
template <typename Generator>
auto streamify(Generator gen_)
{
    return generator2stream<Generator>(gen_);
}

template <typename Input1, typename Input2, typename... Rest>
class make_tuplestream;

//! Combine multiple (at least two) streams into a single tuplestream, the
//! return type of the tuplestream is a std::tuple<> containing the entries
//! of the initial input streams
template <typename Input1, typename Input2, typename... Rest>
class make_tuplestream
{
public:
    using value_type = std::tuple<typename Input1::value_type, typename Input2::value_type, typename Rest::value_type...>;
    using tuple_type = std::tuple<Input1&, Input2&, Rest& ...>;

private:
    tuple_type in;
    value_type current;

public:
    make_tuplestream(Input1& i1, Input2& i2, Rest& ... rest)
        : in(i1, i2, rest...)
    {
        if (!empty())
            current = tlx::vmap_foreach_tuple([](auto& t) { return *t; }, in);
    }

    //! Standard stream method.
    const value_type& operator * () const
    {
        return current;
    }

    const value_type* operator -> () const
    {
        return &current;
    }

    //! Standard stream method.
    make_tuplestream& operator ++ ()
    {
        tlx::call_foreach_tuple([&](auto& t) { ++t; }, in);
        if (!empty())
            current = tlx::vmap_foreach_tuple([](auto& t) { return *t; }, in);
        return *this;
    }

    bool empty() const
    {
        return tlx::fold_left_tuple([](bool a, bool b) { return a || b; }, false,
                                    tlx::vmap_foreach_tuple([](auto& t) { return t.empty(); }, in));
    }
};

template <typename Operation, typename... Inputs>
class transform;

//! Combine multiple (at least two) streams into a single stream, where a
//! functor is applied to all entries.
template <typename Operation, typename... Inputs>
class transform
{
public:
    using value_type = typename Operation::value_type;
    using tuple_type = std::tuple<Inputs& ...>;
    using tuple_value_type = std::tuple<typename Inputs::value_type...>;

private:
    Operation& op;
    tuple_type in;
    value_type current;

public:
    transform(Operation& op_, Inputs& ... in_)
        : op(op_), in(in_...)
    {
        if (!empty())
            current = tlx::apply_tuple(op, tlx::vmap_foreach_tuple([](auto& t) { return *t; }, in));
    }

    //! Standard stream method.
    const value_type& operator * () const
    {
        return current;
    }

    const value_type* operator -> () const
    {
        return &current;
    }

    //! Standard stream method.
    transform& operator ++ ()
    {
        tlx::call_foreach_tuple([&](auto& t) { ++t; }, in);
        if (!empty())
            current = tlx::apply_tuple(op, tlx::vmap_foreach_tuple([](auto& t) { return *t; }, in));
        return *this;
    }

    bool empty() const
    {
        return tlx::fold_left_tuple([](bool a, bool b) { return a || b; }, false,
                                    tlx::vmap_foreach_tuple([](auto& t) { return t.empty(); }, in));
    }
};

/**
 * Counter for creating tuple indexes for example.
 */
template <class ValueType>
struct counter
{
public:
    using value_type = ValueType;

protected:
    value_type m_count;

public:
    explicit counter(const value_type& start = 0)
        : m_count(start)
    { }

    const value_type& operator * () const
    {
        return m_count;
    }

    counter& operator ++ ()
    {
        ++m_count;
        return *this;
    }

    bool empty() const
    {
        return false;
    }
};

/**
 * Concatenates two tuple streams as streamA . streamB
 */
template <class StreamA, class StreamB>
class concatenate
{
public:
    using value_type = typename StreamA::value_type;

private:
    StreamA& A;
    StreamB& B;

public:
    concatenate(StreamA& A_, StreamB& B_) : A(A_), B(B_)
    {
        assert(!A.empty());
        assert(!B.empty());
    }

    const value_type& operator * () const
    {
        assert(!empty());

        if (!A.empty()) {
            return *A;
        }
        else {
            return *B;
        }
    }

    concatenate& operator ++ ()
    {
        assert(!empty());

        if (!A.empty()) {
            ++A;
        }
        else if (!B.empty()) {
            ++B;
        }

        return *this;
    }

    bool empty() const
    {
        return (A.empty() && B.empty());
    }
};

} // namespace stream
} // namespace stxxl

#include <stxxl/bits/stream/choose.h>
#include <stxxl/bits/stream/materialize.h>
#include <stxxl/bits/stream/unique.h>

#endif // !STXXL_STREAM_STREAM_HEADER
