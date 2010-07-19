/***************************************************************************
 *  include/stxxl/bits/stream/stream.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM_HEADER
#define STXXL_STREAM_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/mng/buf_istream.h>
#include <stxxl/bits/mng/buf_ostream.h>
#include <stxxl/bits/common/tuple.h>
#include <stxxl/vector>
#include <stxxl/bits/compat_unique_ptr.h>


#ifndef STXXL_VERBOSE_MATERIALIZE
#define STXXL_VERBOSE_MATERIALIZE STXXL_VERBOSE3
#endif


__STXXL_BEGIN_NAMESPACE

//! \brief Stream package subnamespace
namespace stream
{
    //! \weakgroup streampack Stream package
    //! Package that enables pipelining of consequent sorts
    //! and scans of the external data avoiding the saving the intermediate
    //! results on the disk, e.g. the output of a sort can be directly
    //! fed into a scan procedure without the need to save it on a disk.
    //! All components of the package are contained in the \c stxxl::stream
    //! namespace.
    //!
    //!    STREAM ALGORITHM CONCEPT (Do not confuse with C++ input/output streams)
    //!
    //! \verbatim
    //!
    //!    struct stream_algorithm // stream, pipe, whatever
    //!    {
    //!      typedef some_type value_type;
    //!
    //!      const value_type & operator * () const; // return current element of the stream
    //!      stream_algorithm & operator ++ ();      // go to next element. precondition: empty() == false
    //!      bool empty() const;                     // return true if end of stream is reached
    //!
    //!    };
    //! \endverbatim
    //!
    //! \{


    ////////////////////////////////////////////////////////////////////////
    //     STREAMIFY                                                      //
    ////////////////////////////////////////////////////////////////////////

    //! \brief A model of stream that retrieves the data from an input iterator
    //! For convenience use \c streamify function instead of direct instantiation
    //! of \c iterator2stream .
    template <class InputIterator_>
    class iterator2stream
    {
        InputIterator_ current_, end_;

    public:
        //! \brief Standard stream typedef
        typedef typename std::iterator_traits<InputIterator_>::value_type value_type;

        iterator2stream(InputIterator_ begin, InputIterator_ end) :
            current_(begin), end_(end) { }

        iterator2stream(const iterator2stream & a) : current_(a.current_), end_(a.end_) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return *current_;
        }

        const value_type * operator -> () const
        {
            return &(*current_);
        }

        //! \brief Standard stream method
        iterator2stream & operator ++ ()
        {
            assert(end_ != current_);
            ++current_;
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return (current_ == end_);
        }
    };


    //! \brief Input iterator range to stream converter
    //! \param begin iterator, pointing to the first value
    //! \param end iterator, pointing to the last + 1 position, i.e. beyond the range
    //! \return an instance of a stream object
    template <class InputIterator_>
    iterator2stream<InputIterator_> streamify(InputIterator_ begin, InputIterator_ end)
    {
        return iterator2stream<InputIterator_>(begin, end);
    }

    //! \brief Traits class of \c streamify function
    template <class InputIterator_>
    struct streamify_traits
    {
        //! \brief return type (stream type) of \c streamify for \c InputIterator_
        typedef iterator2stream<InputIterator_> stream_type;
    };

    //! \brief A model of stream that retrieves data from an external \c stxxl::vector iterator.
    //! It is more efficient than generic \c iterator2stream thanks to use of overlapping
    //! For convenience use \c streamify function instead of direct instantiation
    //! of \c vector_iterator2stream .
    template <class InputIterator_>
    class vector_iterator2stream
    {
        InputIterator_ current_, end_;
        typedef buf_istream<typename InputIterator_::block_type,
                            typename InputIterator_::bids_container_iterator> buf_istream_type;

        typedef typename stxxl::compat_unique_ptr<buf_istream_type>::result buf_istream_unique_ptr_type;
        mutable buf_istream_unique_ptr_type in;

        void delete_stream()
        {
            in.reset();  // delete object
        }

    public:
        typedef vector_iterator2stream<InputIterator_> Self_;

        //! \brief Standard stream typedef
        typedef typename std::iterator_traits<InputIterator_>::value_type value_type;

        vector_iterator2stream(InputIterator_ begin, InputIterator_ end, unsigned_type nbuffers = 0) :
            current_(begin), end_(end), in(static_cast<buf_istream_type *>(NULL))
        {
            begin.flush();     // flush container
            typename InputIterator_::bids_container_iterator end_iter = end.bid() + ((end.block_offset()) ? 1 : 0);

            if (end_iter - begin.bid() > 0)
            {
                in.reset(new buf_istream_type(begin.bid(), end_iter, nbuffers ? nbuffers :
                                              (2 * config::get_instance()->disks_number())));

                InputIterator_ cur = begin - begin.block_offset();

                // skip the beginning of the block
                for ( ; cur != begin; ++cur)
                    ++(*in);
            }
        }

        vector_iterator2stream(const Self_ & a) :
            current_(a.current_), end_(a.end_), in(a.in.release()) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return **in;
        }

        const value_type * operator -> () const
        {
            return &(**in);
        }

        //! \brief Standard stream method
        Self_ & operator ++ ()
        {
            assert(end_ != current_);
            ++current_;
            ++(*in);
            if (empty())
                delete_stream();

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return (current_ == end_);
        }
        virtual ~vector_iterator2stream()
        {
            delete_stream();      // not needed actually
        }
    };

    //! \brief Input external \c stxxl::vector iterator range to stream converter
    //! It is more efficient than generic input iterator \c streamify thanks to use of overlapping
    //! \param begin iterator, pointing to the first value
    //! \param end iterator, pointing to the last + 1 position, i.e. beyond the range
    //! \param nbuffers number of blocks used for overlapped reading (0 is default,
    //! which equals to (2 * number_of_disks)
    //! \return an instance of a stream object

    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
    vector_iterator2stream<stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
    streamify(
        stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> begin,
        stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> end,
        unsigned_type nbuffers = 0)
    {
        STXXL_VERBOSE1("streamify for vector_iterator range is called");
        return vector_iterator2stream<stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
                   (begin, end, nbuffers);
    }

    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
    struct streamify_traits<stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
    {
        typedef vector_iterator2stream<stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> > stream_type;
    };

    //! \brief Input external \c stxxl::vector const iterator range to stream converter
    //! It is more efficient than generic input iterator \c streamify thanks to use of overlapping
    //! \param begin const iterator, pointing to the first value
    //! \param end const iterator, pointing to the last + 1 position, i.e. beyond the range
    //! \param nbuffers number of blocks used for overlapped reading (0 is default,
    //! which equals to (2 * number_of_disks)
    //! \return an instance of a stream object

    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
    vector_iterator2stream<stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
    streamify(
        stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> begin,
        stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> end,
        unsigned_type nbuffers = 0)
    {
        STXXL_VERBOSE1("streamify for const_vector_iterator range is called");
        return vector_iterator2stream<stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
                   (begin, end, nbuffers);
    }

    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
    struct streamify_traits<stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
    {
        typedef vector_iterator2stream<stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> > stream_type;
    };


    //! \brief Version of  \c iterator2stream. Switches between \c vector_iterator2stream and \c iterator2stream .
    //!
    //! small range switches between
    //! \c vector_iterator2stream and \c iterator2stream .
    //! iterator2stream is chosen if the input iterator range
    //! is small ( < B )
    template <class InputIterator_>
    class vector_iterator2stream_sr
    {
        vector_iterator2stream<InputIterator_> * vec_it_stream;
        iterator2stream<InputIterator_> * it_stream;

        typedef typename InputIterator_::block_type block_type;

    public:
        typedef vector_iterator2stream_sr<InputIterator_> Self_;

        //! \brief Standard stream typedef
        typedef typename std::iterator_traits<InputIterator_>::value_type value_type;

        vector_iterator2stream_sr(InputIterator_ begin, InputIterator_ end, unsigned_type nbuffers = 0)
        {
            if (end - begin < block_type::size)
            {
                STXXL_VERBOSE1("vector_iterator2stream_sr::vector_iterator2stream_sr: Choosing iterator2stream<InputIterator_>");
                it_stream = new iterator2stream<InputIterator_>(begin, end);
                vec_it_stream = NULL;
            }
            else
            {
                STXXL_VERBOSE1("vector_iterator2stream_sr::vector_iterator2stream_sr: Choosing vector_iterator2stream<InputIterator_>");
                it_stream = NULL;
                vec_it_stream = new vector_iterator2stream<InputIterator_>(begin, end, nbuffers);
            }
        }

        vector_iterator2stream_sr(const Self_ & a) : vec_it_stream(a.vec_it_stream), it_stream(a.it_stream) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            if (it_stream)
                return **it_stream;

            return **vec_it_stream;
        }

        const value_type * operator -> () const
        {
            if (it_stream)
                return &(**it_stream);

            return &(**vec_it_stream);
        }

        //! \brief Standard stream method
        Self_ & operator ++ ()
        {
            if (it_stream)
                ++(*it_stream);

            else
                ++(*vec_it_stream);


            return *this;
        }

        //! \brief Standard stream method
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

    //! \brief Version of  \c streamify. Switches from \c vector_iterator2stream to \c iterator2stream for small ranges.
    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
    vector_iterator2stream_sr<stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
    streamify_sr(
        stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> begin,
        stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> end,
        unsigned_type nbuffers = 0)
    {
        STXXL_VERBOSE1("streamify_sr for vector_iterator range is called");
        return vector_iterator2stream_sr<stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
                   (begin, end, nbuffers);
    }

    //! \brief Version of  \c streamify. Switches from \c vector_iterator2stream to \c iterator2stream for small ranges.
    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
    vector_iterator2stream_sr<stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
    streamify_sr(
        stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> begin,
        stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> end,
        unsigned_type nbuffers = 0)
    {
        STXXL_VERBOSE1("streamify_sr for const_vector_iterator range is called");
        return vector_iterator2stream_sr<stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> >
                   (begin, end, nbuffers);
    }


    ////////////////////////////////////////////////////////////////////////
    //     MATERIALIZE                                                    //
    ////////////////////////////////////////////////////////////////////////

    //! \brief Stores consecutively stream content to an output iterator
    //! \param in stream to be stored used as source
    //! \param out output iterator used as destination
    //! \return value of the output iterator after all increments,
    //! i.e. points to the first unwritten value
    //! \pre Output (range) is large enough to hold the all elements in the input stream
    template <class OutputIterator_, class StreamAlgorithm_>
    OutputIterator_ materialize(StreamAlgorithm_ & in, OutputIterator_ out)
    {
        STXXL_VERBOSE_MATERIALIZE(STXXL_PRETTY_FUNCTION_NAME);
        while (!in.empty())
        {
            *out = *in;
            ++out;
            ++in;
        }
        return out;
    }


    //! \brief Stores consecutively stream content to an output iterator range \b until end of the stream or end of the iterator range is reached
    //! \param in stream to be stored used as source
    //! \param outbegin output iterator used as destination
    //! \param outend output end iterator, pointing beyond the output range
    //! \return value of the output iterator after all increments,
    //! i.e. points to the first unwritten value
    //! \pre Output range is large enough to hold the all elements in the input stream
    //!
    //! This function is useful when you do not know the length of the stream beforehand.
    template <class OutputIterator_, class StreamAlgorithm_>
    OutputIterator_ materialize(StreamAlgorithm_ & in, OutputIterator_ outbegin, OutputIterator_ outend)
    {
        STXXL_VERBOSE_MATERIALIZE(STXXL_PRETTY_FUNCTION_NAME);
        while ((!in.empty()) && outend != outbegin)
        {
            *outbegin = *in;
            ++outbegin;
            ++in;
        }
        return outbegin;
    }


    //! \brief Stores consecutively stream content to an output \c stxxl::vector iterator \b until end of the stream or end of the iterator range is reached
    //! \param in stream to be stored used as source
    //! \param outbegin output \c stxxl::vector iterator used as destination
    //! \param outend output end iterator, pointing beyond the output range
    //! \param nbuffers number of blocks used for overlapped writing (0 is default,
    //! which equals to (2 * number_of_disks)
    //! \return value of the output iterator after all increments,
    //! i.e. points to the first unwritten value
    //! \pre Output range is large enough to hold the all elements in the input stream
    //!
    //! This function is useful when you do not know the length of the stream beforehand.
    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_, class StreamAlgorithm_>
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>
    materialize(StreamAlgorithm_ & in,
                stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> outbegin,
                stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> outend,
                unsigned_type nbuffers = 0)
    {
        STXXL_VERBOSE_MATERIALIZE(STXXL_PRETTY_FUNCTION_NAME);
        typedef stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> ExtIterator;
        typedef stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> ConstExtIterator;
        typedef buf_ostream<typename ExtIterator::block_type, typename ExtIterator::bids_container_iterator> buf_ostream_type;


        while (outbegin.block_offset()) //  go to the beginning of the block
        //  of the external vector
        {
            if (in.empty() || outbegin == outend)
                return outbegin;

            *outbegin = *in;
            ++outbegin;
            ++in;
        }

        if (nbuffers == 0)
            nbuffers = 2 * config::get_instance()->disks_number();

        outbegin.flush(); // flush container

        // create buffered write stream for blocks
        buf_ostream_type outstream(outbegin.bid(), nbuffers);

        assert(outbegin.block_offset() == 0);

        // delay calling block_externally_updated() until the block is
        // completely filled (and written out) in outstream
        ConstExtIterator prev_block = outbegin;

        while (!in.empty() && outend != outbegin)
        {
            if (outbegin.block_offset() == 0) {
                if (prev_block != outbegin) {
                    prev_block.block_externally_updated();
                    prev_block = outbegin;
                }
            }

            *outstream = *in;
            ++outbegin;
            ++outstream;
            ++in;
        }

        ConstExtIterator const_out = outbegin;

        while (const_out.block_offset()) // filling the rest of the block
        {
            *outstream = *const_out;
            ++const_out;
            ++outstream;
        }

        if (prev_block != outbegin)
            prev_block.block_externally_updated();

        outbegin.flush();

        return outbegin;
    }


    //! \brief Stores consecutively stream content to an output \c stxxl::vector iterator
    //! \param in stream to be stored used as source
    //! \param out output \c stxxl::vector iterator used as destination
    //! \param nbuffers number of blocks used for overlapped writing (0 is default,
    //! which equals to (2 * number_of_disks)
    //! \return value of the output iterator after all increments,
    //! i.e. points to the first unwritten value
    //! \pre Output (range) is large enough to hold the all elements in the input stream
    template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
              unsigned BlkSize_, typename PgTp_, unsigned PgSz_, class StreamAlgorithm_>
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>
    materialize(StreamAlgorithm_ & in,
                stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> out,
                unsigned_type nbuffers = 0)
    {
        STXXL_VERBOSE_MATERIALIZE(STXXL_PRETTY_FUNCTION_NAME);
        typedef stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> ExtIterator;
        typedef stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> ConstExtIterator;
        typedef buf_ostream<typename ExtIterator::block_type, typename ExtIterator::bids_container_iterator> buf_ostream_type;

        // on the I/O complexity of "materialize":
        // crossing block boundary causes O(1) I/Os
        // if you stay in a block, then materialize function accesses only the cache of the
        // vector (only one block indeed), amortized complexity should apply here

        while (out.block_offset()) //  go to the beginning of the block
        //  of the external vector
        {
            if (in.empty())
                return out;

            *out = *in;
            ++out;
            ++in;
        }

        if (nbuffers == 0)
            nbuffers = 2 * config::get_instance()->disks_number();


        out.flush(); // flush container

        // create buffered write stream for blocks
        buf_ostream_type outstream(out.bid(), nbuffers);

        assert(out.block_offset() == 0);

        // delay calling block_externally_updated() until the block is
        // completely filled (and written out) in outstream
        ConstExtIterator prev_block = out;

        while (!in.empty())
        {
            if (out.block_offset() == 0) {
                if (prev_block != out) {
                    prev_block.block_externally_updated();
                    prev_block = out;
                }
            }

            // tells the vector that the block was modified
            *outstream = *in;
            ++out;
            ++outstream;
            ++in;
        }

        ConstExtIterator const_out = out;

        while (const_out.block_offset())
        {
            *outstream = *const_out;             // might cause I/Os for loading the page that
            ++const_out;                         // contains data beyond out
            ++outstream;
        }

        if (prev_block != out)
            prev_block.block_externally_updated();

        out.flush();

        return out;
    }


    ////////////////////////////////////////////////////////////////////////
    //     GENERATE                                                       //
    ////////////////////////////////////////////////////////////////////////

    //! \brief A model of stream that outputs data from an adaptable generator functor
    //! For convenience use \c streamify function instead of direct instantiation
    //! of \c generator2stream .
    template <class Generator_, typename T = typename Generator_::value_type>
    class generator2stream
    {
    public:
        //! \brief Standard stream typedef
        typedef T value_type;

    private:
        Generator_ gen_;
        value_type current_;

    public:
        generator2stream(Generator_ g) :
            gen_(g), current_(gen_()) { }

        generator2stream(const generator2stream & a) : gen_(a.gen_), current_(a.current_) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current_;
        }

        const value_type * operator -> () const
        {
            return &current_;
        }

        //! \brief Standard stream method
        generator2stream & operator ++ ()
        {
            current_ = gen_();
            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return false;
        }
    };

    //! \brief Adaptable generator to stream converter
    //! \param gen_ generator object
    //! \return an instance of a stream object
    template <class Generator_>
    generator2stream<Generator_> streamify(Generator_ gen_)
    {
        return generator2stream<Generator_>(gen_);
    }


    ////////////////////////////////////////////////////////////////////////
    //     TRANSFORM                                                      //
    ////////////////////////////////////////////////////////////////////////

    struct Stopper { };

    //! \brief Processes (up to) 6 input streams using given operation functor
    //!
    //! Template parameters:
    //! - \c Operation_ type of the operation (type of an
    //! adaptable functor that takes 6 parameters)
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! - \c Input4_ type of the 4th input
    //! - \c Input5_ type of the 5th input
    //! - \c Input6_ type of the 6th input
    template <class Operation_,
              class Input1_,
              class Input2_ = Stopper,
              class Input3_ = Stopper,
              class Input4_ = Stopper,
              class Input5_ = Stopper,
              class Input6_ = Stopper
              >
    class transform
    {
        Operation_ op;
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;
        Input4_ & i4;
        Input5_ & i5;
        Input6_ & i6;

    public:
        //! \brief Standard stream typedef
        typedef typename Operation_::value_type value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        transform(Operation_ o, Input1_ & i1_, Input2_ & i2_, Input3_ & i3_, Input4_ & i4_,
                  Input5_ & i5_, Input5_ & i6_) :
            op(o), i1(i1_), i2(i2_), i3(i3_), i4(i4_), i5(i5_), i6(i6_),
            current(op(*i1, *i2, *i3, *i4, *i5, *i6)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        transform & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;
            ++i4;
            ++i5;
            ++i6;

            if (!empty())
                current = op(*i1, *i2, *i3, *i4, *i5, *i6);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty() ||
                   i4.empty() || i5.empty() || i6.empty();
        }
    };

    // Specializations

    ////////////////////////////////////////////////////////////////////////
    //     TRANSFORM (1 input stream)                                     //
    ////////////////////////////////////////////////////////////////////////

    //! \brief Processes an input stream using given operation functor
    //!
    //! Template parameters:
    //! - \c Operation_ type of the operation (type of an
    //! adaptable functor that takes 1 parameter)
    //! - \c Input1_ type of the input
    //! \remark This is a specialization of \c transform .
    template <class Operation_,
              class Input1_
              >
    class transform<Operation_, Input1_, Stopper, Stopper, Stopper, Stopper, Stopper>
    {
        Operation_ op;
        Input1_ & i1;

    public:
        //! \brief Standard stream typedef
        typedef typename Operation_::value_type value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        transform(Operation_ o, Input1_ & i1_) : op(o), i1(i1_),
                                                 current(op(*i1)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        transform & operator ++ ()
        {
            ++i1;
            if (!empty())
                current = op(*i1);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty();
        }
    };


    ////////////////////////////////////////////////////////////////////////
    //     TRANSFORM (2 input streams)                                    //
    ////////////////////////////////////////////////////////////////////////

    //! \brief Processes 2 input streams using given operation functor
    //!
    //! Template parameters:
    //! - \c Operation_ type of the operation (type of an
    //! adaptable functor that takes 2 parameters)
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! \remark This is a specialization of \c transform .
    template <class Operation_,
              class Input1_,
              class Input2_
              >
    class transform<Operation_, Input1_, Input2_, Stopper, Stopper, Stopper, Stopper>
    {
        Operation_ op;
        Input1_ & i1;
        Input2_ & i2;

    public:
        //! \brief Standard stream typedef
        typedef typename Operation_::value_type value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        transform(Operation_ o, Input1_ & i1_, Input2_ & i2_) : op(o), i1(i1_), i2(i2_),
                                                                current(op(*i1, *i2)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        transform & operator ++ ()
        {
            ++i1;
            ++i2;
            if (!empty())
                current = op(*i1, *i2);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty();
        }
    };


    ////////////////////////////////////////////////////////////////////////
    //     TRANSFORM (3 input streams)                                    //
    ////////////////////////////////////////////////////////////////////////

    //! \brief Processes 3 input streams using given operation functor
    //!
    //! Template parameters:
    //! - \c Operation_ type of the operation (type of an
    //! adaptable functor that takes 3 parameters)
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! \remark This is a specialization of \c transform .
    template <class Operation_,
              class Input1_,
              class Input2_,
              class Input3_
              >
    class transform<Operation_, Input1_, Input2_, Input3_, Stopper, Stopper, Stopper>
    {
        Operation_ op;
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;

    public:
        //! \brief Standard stream typedef
        typedef typename Operation_::value_type value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        transform(Operation_ o, Input1_ & i1_, Input2_ & i2_, Input3_ & i3_) :
            op(o), i1(i1_), i2(i2_), i3(i3_),
            current(op(*i1, *i2, *i3)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        transform & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;
            if (!empty())
                current = op(*i1, *i2, *i3);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty();
        }
    };


    ////////////////////////////////////////////////////////////////////////
    //     TRANSFORM (4 input streams)                                    //
    ////////////////////////////////////////////////////////////////////////

    //! \brief Processes 4 input streams using given operation functor
    //!
    //! Template parameters:
    //! - \c Operation_ type of the operation (type of an
    //! adaptable functor that takes 4 parameters)
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! - \c Input4_ type of the 4th input
    //! \remark This is a specialization of \c transform .
    template <class Operation_,
              class Input1_,
              class Input2_,
              class Input3_,
              class Input4_
              >
    class transform<Operation_, Input1_, Input2_, Input3_, Input4_, Stopper, Stopper>
    {
        Operation_ op;
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;
        Input4_ & i4;

    public:
        //! \brief Standard stream typedef
        typedef typename Operation_::value_type value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        transform(Operation_ o, Input1_ & i1_, Input2_ & i2_, Input3_ & i3_, Input4_ & i4_) :
            op(o), i1(i1_), i2(i2_), i3(i3_), i4(i4_),
            current(op(*i1, *i2, *i3, *i4)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        transform & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;
            ++i4;
            if (!empty())
                current = op(*i1, *i2, *i3, *i4);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty() || i4.empty();
        }
    };


    ////////////////////////////////////////////////////////////////////////
    //     TRANSFORM (5 input streams)                                    //
    ////////////////////////////////////////////////////////////////////////

    //! \brief Processes 5 input streams using given operation functor
    //!
    //! Template parameters:
    //! - \c Operation_ type of the operation (type of an
    //! adaptable functor that takes 5 parameters)
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! - \c Input4_ type of the 4th input
    //! - \c Input5_ type of the 5th input
    //! \remark This is a specialization of \c transform .
    template <class Operation_,
              class Input1_,
              class Input2_,
              class Input3_,
              class Input4_,
              class Input5_
              >
    class transform<Operation_, Input1_, Input2_, Input3_, Input4_, Input5_, Stopper>
    {
        Operation_ op;
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;
        Input4_ & i4;
        Input5_ & i5;

    public:
        //! \brief Standard stream typedef
        typedef typename Operation_::value_type value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        transform(Operation_ o, Input1_ & i1_, Input2_ & i2_, Input3_ & i3_, Input4_ & i4_,
                  Input5_ & i5_) :
            op(o), i1(i1_), i2(i2_), i3(i3_), i4(i4_), i5(i5_),
            current(op(*i1, *i2, *i3, *i4, *i5)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        transform & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;
            ++i4;
            ++i5;
            if (!empty())
                current = op(*i1, *i2, *i3, *i4, *i5);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty() || i4.empty() || i5.empty();
        }
    };


    ////////////////////////////////////////////////////////////////////////
    //     MAKE TUPLE                                                     //
    ////////////////////////////////////////////////////////////////////////

    //! \brief Creates stream of 6-tuples from 6 input streams
    //!
    //! Template parameters:
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! - \c Input4_ type of the 4th input
    //! - \c Input5_ type of the 5th input
    //! - \c Input6_ type of the 6th input
    template <class Input1_,
              class Input2_,
              class Input3_ = Stopper,
              class Input4_ = Stopper,
              class Input5_ = Stopper,
              class Input6_ = Stopper
              >
    class make_tuple
    {
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;
        Input4_ & i4;
        Input5_ & i5;
        Input6_ & i6;

    public:
        //! \brief Standard stream typedef
        typedef typename stxxl::tuple<
            typename Input1_::value_type,
            typename Input2_::value_type,
            typename Input3_::value_type,
            typename Input4_::value_type,
            typename Input5_::value_type,
            typename Input6_::value_type
            > value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        make_tuple(
            Input1_ & i1_,
            Input2_ & i2_,
            Input3_ & i3_,
            Input4_ & i4_,
            Input5_ & i5_,
            Input6_ & i6_
            ) :
            i1(i1_), i2(i2_), i3(i3_), i4(i4_), i5(i5_), i6(i6_),
            current(value_type(*i1, *i2, *i3, *i4, *i5, *i6)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        make_tuple & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;
            ++i4;
            ++i5;
            ++i6;

            if (!empty())
                current = value_type(*i1, *i2, *i3, *i4, *i5, *i6);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty() ||
                   i4.empty() || i5.empty() || i6.empty();
        }
    };


    //! \brief Creates stream of 2-tuples (pairs) from 2 input streams
    //!
    //! Template parameters:
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! \remark A specialization of \c make_tuple .
    template <class Input1_,
              class Input2_
              >
    class make_tuple<Input1_, Input2_, Stopper, Stopper, Stopper, Stopper>
    {
        Input1_ & i1;
        Input2_ & i2;

    public:
        //! \brief Standard stream typedef
        typedef typename stxxl::tuple<
            typename Input1_::value_type,
            typename Input2_::value_type
            > value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        make_tuple(
            Input1_ & i1_,
            Input2_ & i2_
            ) :
            i1(i1_), i2(i2_)
        {
            if (!empty())
            {
                current = value_type(*i1, *i2);
            }
        }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        make_tuple & operator ++ ()
        {
            ++i1;
            ++i2;

            if (!empty())
                current = value_type(*i1, *i2);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty();
        }
    };

    //! \brief Creates stream of 3-tuples from 3 input streams
    //!
    //! Template parameters:
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! \remark A specialization of \c make_tuple .
    template <class Input1_,
              class Input2_,
              class Input3_
              >
    class make_tuple<Input1_, Input2_, Input3_, Stopper, Stopper, Stopper>
    {
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;

    public:
        //! \brief Standard stream typedef
        typedef typename stxxl::tuple<
            typename Input1_::value_type,
            typename Input2_::value_type,
            typename Input3_::value_type
            > value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        make_tuple(
            Input1_ & i1_,
            Input2_ & i2_,
            Input3_ & i3_
            ) :
            i1(i1_), i2(i2_), i3(i3_),
            current(value_type(*i1, *i2, *i3)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        make_tuple & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;

            if (!empty())
                current = value_type(*i1, *i2, *i3);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty();
        }
    };

    //! \brief Creates stream of 4-tuples from 4 input streams
    //!
    //! Template parameters:
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! - \c Input4_ type of the 4th input
    //! \remark A specialization of \c make_tuple .
    template <class Input1_,
              class Input2_,
              class Input3_,
              class Input4_
              >
    class make_tuple<Input1_, Input2_, Input3_, Input4_, Stopper, Stopper>
    {
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;
        Input4_ & i4;

    public:
        //! \brief Standard stream typedef
        typedef typename stxxl::tuple<
            typename Input1_::value_type,
            typename Input2_::value_type,
            typename Input3_::value_type,
            typename Input4_::value_type
            > value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        make_tuple(
            Input1_ & i1_,
            Input2_ & i2_,
            Input3_ & i3_,
            Input4_ & i4_
            ) :
            i1(i1_), i2(i2_), i3(i3_), i4(i4_),
            current(value_type(*i1, *i2, *i3, *i4)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        make_tuple & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;
            ++i4;

            if (!empty())
                current = value_type(*i1, *i2, *i3, *i4);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty() ||
                   i4.empty();
        }
    };

    //! \brief Creates stream of 5-tuples from 5 input streams
    //!
    //! Template parameters:
    //! - \c Input1_ type of the 1st input
    //! - \c Input2_ type of the 2nd input
    //! - \c Input3_ type of the 3rd input
    //! - \c Input4_ type of the 4th input
    //! - \c Input5_ type of the 5th input
    //! \remark A specialization of \c make_tuple .
    template <
        class Input1_,
        class Input2_,
        class Input3_,
        class Input4_,
        class Input5_
        >
    class make_tuple<Input1_, Input2_, Input3_, Input4_, Input5_, Stopper>
    {
        Input1_ & i1;
        Input2_ & i2;
        Input3_ & i3;
        Input4_ & i4;
        Input5_ & i5;

    public:
        //! \brief Standard stream typedef
        typedef typename stxxl::tuple<
            typename Input1_::value_type,
            typename Input2_::value_type,
            typename Input3_::value_type,
            typename Input4_::value_type,
            typename Input5_::value_type
            > value_type;

    private:
        value_type current;

    public:
        //! \brief Construction
        make_tuple(
            Input1_ & i1_,
            Input2_ & i2_,
            Input3_ & i3_,
            Input4_ & i4_,
            Input5_ & i5_
            ) :
            i1(i1_), i2(i2_), i3(i3_), i4(i4_), i5(i5_),
            current(value_type(*i1, *i2, *i3, *i4, *i5)) { }

        //! \brief Standard stream method
        const value_type & operator * () const
        {
            return current;
        }

        const value_type * operator -> () const
        {
            return &current;
        }

        //! \brief Standard stream method
        make_tuple & operator ++ ()
        {
            ++i1;
            ++i2;
            ++i3;
            ++i4;
            ++i5;

            if (!empty())
                current = value_type(*i1, *i2, *i3, *i4, *i5);

            return *this;
        }

        //! \brief Standard stream method
        bool empty() const
        {
            return i1.empty() || i2.empty() || i3.empty() ||
                   i4.empty() || i5.empty();
        }
    };


//! \}
}

__STXXL_END_NAMESPACE


#include <stxxl/bits/stream/choose.h>
#include <stxxl/bits/stream/unique.h>


#endif // !STXXL_STREAM_HEADER
// vim: et:ts=4:sw=4
