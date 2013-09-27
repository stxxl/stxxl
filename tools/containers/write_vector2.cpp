/***************************************************************************
 *  containers/write_vector2.cpp
 *
 *  copies a file to another file
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/vector>
#include <stxxl/stream>

typedef unsigned char my_type;

using stxxl::syscall_file;
using stxxl::file;

int main(int argc, char * argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " input_file output_file " << std::endl;
        return -1;
    }

    unlink(argv[2]);                                                // delete output file

    stxxl::timer tm;
    tm.start();

    // Input file object
    syscall_file InputFile(argv[1], file::RDONLY | file::DIRECT);
    // Output file object
    syscall_file OutputFile(argv[2], file::RDWR | file::CREAT | file::DIRECT);

    typedef stxxl::vector<my_type> vector_type;

    std::cout << "Copying file " << argv[1] << " to " << argv[2] << std::endl;

    vector_type InputVector(&InputFile);               // InputVector is mapped to InputFile
    vector_type OutputVector(&OutputFile);             // OutputVector is mapped to OutputFile
    OutputVector.resize(InputVector.size());

    std::cout << "File " << argv[1] << " has size " << InputVector.size() << " bytes." << std::endl;

    stxxl::stream::vector_iterator2stream<vector_type::const_iterator> input(InputVector.begin(), InputVector.end());
    stxxl::vector_bufwriter<vector_type> writer(OutputVector.begin());

    while (!input.empty()) // iterate through InputVector
    {
        writer << *input;
        ++input;
    }
    writer.finish();                                            // flush buffers

    std::cout << "Copied in " << tm.seconds() << " at "
              << (double)InputVector.size() / tm.seconds() / 1024 / 1024 << " MiB/s" << std::endl;

    return 0;
}
