/***************************************************************************
 *  containers/write_vector2.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/vector>
#include <stxxl/bits/mng/buf_ostream.h>

// efficiently writes data into an stxxl::vector with overlapping of I/O and
// computation
template <class ExtIterator>
class write_vector
{
    typedef typename ExtIterator::size_type size_type;
    typedef typename ExtIterator::value_type value_type;
    typedef typename ExtIterator::block_type block_type;
    typedef typename ExtIterator::const_iterator ConstExtIterator;
    typedef stxxl::buf_ostream<block_type, typename ExtIterator::bids_container_iterator> buf_ostream_type;

    ExtIterator it;
    unsigned nbuffers;
    buf_ostream_type * outstream;

public:
    write_vector(ExtIterator begin,
                 unsigned nbuffers_             // buffers to use for overlapping (>=2 recommended)
                 ) : it(begin), nbuffers(nbuffers_)
    {
        outstream = new buf_ostream_type(it.bid(), nbuffers);
    }

    value_type & operator * ()
    {
        if (it.block_offset() == 0)
            it.touch();
        // tells the vector that the block was modified
        return **outstream;
    }

    write_vector & operator ++ ()
    {
        ++it;
        ++(*outstream);
        return *this;
    }

    void flush()
    {
        ConstExtIterator const_out = it;

        while (const_out.block_offset())
        {
            **outstream = *const_out;           // might cause I/Os for loading the page that
            ++const_out;                        // contains data beyond out
            ++(*outstream);
        }

        it.flush();
        delete outstream;
        outstream = NULL;
    }

    virtual ~write_vector()
    {
        if (outstream)
            flush();
    }
};

typedef unsigned char my_type;

// copies a file to another file

using stxxl::syscall_file;
using stxxl::file;

int main(int argc, char * argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " input_file output_file " << std::endl;
        return 0;
    }

    unlink(argv[2]);                                                // delete output file

    syscall_file InputFile(argv[1], file::RDONLY);                  // Input file object
    syscall_file OutputFile(argv[2], file::RDWR | file::CREAT);     // Output file object

    typedef stxxl::vector<my_type> vector_type;

    std::cout << "Copying file " << argv[1] << " to " << argv[2] << std::endl;

    vector_type InputVector(&InputFile);               // InputVector is mapped to InputFile
    vector_type OutputVector(&OutputFile);             // OutputVector is mapped to OutputFile
    OutputVector.resize(InputVector.size());

    std::cout << "File " << argv[1] << " has size " << InputVector.size() << " bytes." << std::endl;

    vector_type::const_iterator it = InputVector.begin();           // creating const iterator

    write_vector<vector_type::iterator> Writer(OutputVector.begin(), 2);

    for ( ; it != InputVector.end(); ++it, ++Writer)                // iterate through InputVector
    {
        *Writer = *it;
    }
    Writer.flush();                                                 // flush buffers


    return 0;
}
