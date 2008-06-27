/***************************************************************************
 *  containers/write_vector.cpp
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
template <class VectorType>
class write_vector
{
    typedef VectorType vector_type;
    typedef typename vector_type::size_type size_type;
    typedef typename vector_type::value_type value_type;
    typedef typename vector_type::block_type block_type;
    typedef typename vector_type::iterator ExtIterator;
    typedef typename vector_type::const_iterator ConstExtIterator;
    typedef stxxl::buf_ostream<block_type, typename ExtIterator::bids_container_iterator> buf_ostream_type;

    vector_type & Vec;
    size_type RealSize;
    unsigned nbuffers;
    buf_ostream_type * outstream;

public:
    write_vector(vector_type & Vec_,
                 unsigned nbuffers_             // buffers to use for overlapping (>=2 recommended)
                 ) : Vec(Vec_), RealSize(0), nbuffers(nbuffers_)
    {
        assert(Vec.empty());                    // precondition: Vec is empty
        Vec.resize(2 * block_type::size);
        outstream = new buf_ostream_type(Vec.begin().bid(), nbuffers);
    }

    void push_back(const value_type & val)
    {
        ++RealSize;
        if (Vec.size() < RealSize)
        {
            // double the size of the array
            delete outstream;             // flush overlap buffers
            Vec.resize(2 * Vec.size());
            outstream = new buf_ostream_type((Vec.begin() + RealSize - 1).bid(), nbuffers);
        }
        ExtIterator it = Vec.begin() + RealSize - 1;
        if (it.block_offset() == 0)
            it.touch();
        // tells the vector that the block was modified)
        **outstream = val;
        ++(*outstream);
    }

    void finish()
    {
        ExtIterator out = Vec.begin() + RealSize;
        ConstExtIterator const_out = out;

        while (const_out.block_offset())
        {
            **outstream = *const_out;           // might cause I/Os for loading the page that
            ++const_out;                        // contains data beyond out
            ++(*outstream);
        }

        out.flush();
        delete outstream;
        outstream = NULL;
        Vec.resize(RealSize);
    }

    virtual ~write_vector()
    {
        if (outstream)
            finish();
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
        return 1;
    }

    unlink(argv[2]);                                                // delete output file

    syscall_file InputFile(argv[1], file::RDONLY);                  // Input file object
    syscall_file OutputFile(argv[2], file::RDWR | file::CREAT);     // Output file object

    typedef stxxl::vector<my_type> vector_type;

    std::cout << "Copying file " << argv[1] << " to " << argv[2] << std::endl;

    vector_type InputVector(&InputFile);                            // InputVector is mapped to InputFile
    vector_type OutputVector(&OutputFile);                          // OutputVector is mapped to OutputFile

    std::cout << "File " << argv[1] << " has size " << InputVector.size() << " bytes." << std::endl;

    vector_type::const_iterator it = InputVector.begin();           // creating const iterator

    write_vector<vector_type> Writer(OutputVector, 6);

    for ( ; it != InputVector.end(); ++it)                          // iterate through InputVector
    {
        Writer.push_back(*it);                                      // add the value pointed by 'it' to OutputVector
    }
    Writer.finish();                                                // flush buffers


    return 0;
}
