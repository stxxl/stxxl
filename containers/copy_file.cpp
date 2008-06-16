/***************************************************************************
 *  containers/copy_file.cpp
 *
 *  copies a file to another file
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/vector>

typedef unsigned char my_type;


using stxxl::syscall_file;
using stxxl::file;

int main(int argc, char * argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " input_file output_file " << std::endl;
        return 0;
    }

    unlink(argv[2]);             // delete output file

    syscall_file InputFile(argv[1], file::RDONLY);            // Input file object
    syscall_file OutputFile(argv[2], file::RDWR | file::CREAT); // Output file object

    typedef stxxl::vector<my_type> vector_type;

    std::cout << "Copying file " << argv[1] << " to " << argv[2] << std::endl;

    vector_type InputVector(&InputFile);               // InputVector is mapped to InputFile
    vector_type OutputVector(&OutputFile);             // OutputVector is mapped to OutputFile

    std::cout << "File " << argv[1] << " has size " << InputVector.size() << " bytes." << std::endl;

    vector_type::const_iterator it = InputVector.begin();       // creating const iterator


    for ( ; it != InputVector.end(); ++it)       // iterate through InputVector
    {
        OutputVector.push_back(*it);                 // add the value pointed by 'it' to OutputVector
    }


    return 0;
}
