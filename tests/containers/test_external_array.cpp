/***************************************************************************
 *  tests/containers/test_external_array.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/ppq_external_array.cpp
//! This is an example of how to use \c stxxl::external_array

#include <cstddef>
#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/containers/ppq_external_array.h>
#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/timer>

typedef std::pair<size_t,size_t> value_type;

static const size_t printmod = 16 * 1024 * 1024;
static inline void progress(const char* text, size_t i, size_t nelements)
{
    if ((i % printmod) == 0) {
        STXXL_MSG(text << " " << i << " (" << std::setprecision(5) << (static_cast<double>(i)* 100. / static_cast<double>(nelements)) << " %)");
    }
}

//! Run test.
//! \param volume Volume
//! \param numeas Number of external arrays to fill in parallel
//! \param numwbs Number of write buffer blocks for each external array.
int run_test(size_t volume, size_t numeas, size_t numwbs) {
    
    typedef typename stxxl::external_array<value_type,STXXL_DEFAULT_BLOCK_SIZE(value_type),stxxl::striping> ea;
    size_t num_elements=volume/sizeof(value_type);
    num_elements = (num_elements/numeas)*numeas;
    
    std::vector<ea*> eas(numeas);
    
    for (size_t i=0; i<numeas; ++i) {
        eas[i] = new ea(num_elements/numeas,1,numwbs);
    }

    STXXL_MSG("Insert elements 0 to "<<num_elements-1);

    {
        stxxl::scoped_print_timer timer("Filling", volume);
        
        #if STXXL_PARALLEL
        #pragma omp parallel for
        #endif
        for (size_t j=0; j<numeas; ++j) {
            for (size_t i=0; i<num_elements/numeas; ++i) {
                if (numeas<2) progress("Inserting element", i+j*numeas, num_elements);
                *(eas[j]) << std::make_pair(i,i);
            }
        }
        
    }
    

    {
        stxxl::scoped_print_timer timer("Finishing write phase");
        for (unsigned j=0; j<numeas; ++j) {
            eas[j]->finish_write_phase();
        }
    }

    {
        stxxl::scoped_print_timer timer("Reading and checking", volume);
        
        #if STXXL_PARALLEL
        #pragma omp parallel for
        #endif
        for (size_t j=0; j<numeas; ++j) {
            
            size_t counter = 0;
            size_t block_counter = 0;

            ea * a = eas[j];
            while (!a->empty()) {
                a->wait_for_first_block();
                for (ea::iterator i=a->begin_block(); i!=a->end_block(); ++i) {
                    if (i->first != counter) {
                        STXXL_ERRMSG("Read error at index " << counter << ": " << i->first << " != " << counter);
                        abort();
                    }
                    counter++;
                }
                a->remove_first_block();
                a->wait_for_first_block();
                ++block_counter;
            }
            
            STXXL_CHECK_EQUAL(counter,num_elements/numeas);
            
        }
        
    }
    
    return EXIT_SUCCESS;

}

int main(int argc, char** argv)
{

    stxxl::uint64 volume = 2L * 1024 * 1024 * 1024;
    unsigned numeas = 1;
    unsigned numwbs = 14;

    stxxl::cmdline_parser cp;
    cp.set_description("STXXL external array test");
    cp.set_author("Thomas Keh <thomas.keh@student.kit.edu>");
    cp.add_bytes('v', "volume", "Volume to fill into the external array, default: 2 GiB", volume);
    cp.add_uint('n', "numarrays", "Number of external arrays to fill in parallel, default: 1", numeas);
    cp.add_uint('w', "numwritebuffers", "Number of write buffer blocks, default: 14", numwbs);

    if (!cp.process(argc, argv))
        return EXIT_FAILURE;

    STXXL_MEMDUMP(volume);
    STXXL_MEMDUMP(sizeof(value_type));
    STXXL_VARDUMP(numeas);
    STXXL_VARDUMP(numwbs);
    
    return run_test(volume,numeas,numwbs);
    
}
