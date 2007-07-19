/***************************************************************************
 *            test.cpp
 *
 *  Sat Aug 24 23:52:27 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "stxxl/io"
#include "stxxl/random"
#include <cmath>

using namespace stxxl;

// Tests sim_disk file implementation
// must be run on in-memory swap partition !


int main()
{
    stxxl::int64 disk_size = stxxl::int64(1024 * 1024) * 1024 * 40;
    std::cout << sizeof(void *) << std::endl;
    const int block_size = 4 * 1024 * 1024;
    char * buffer = new char[block_size];
    const char * paths[2] = { "/tmp/data1", "/tmp/data2" };
    sim_disk_file file1(paths[0], file::CREAT | file::RDWR /* | file::DIRECT */, 0);
    file1.set_size(disk_size);

    sim_disk_file file2(paths[1], file::CREAT | file::RDWR /* | file::DIRECT */, 1);
    file2.set_size(disk_size);

    unsigned i = 0;

    stxxl::int64 pos = 0;

    request_ptr req;

    STXXL_MSG("Estimated time:" << block_size / double (AVERAGE_SPEED));
    STXXL_MSG("Sequential write");

    for (i = 0; i < 40; i++)
    {
        double begin = stxxl_timestamp();
        req = file1.awrite(buffer, pos, block_size, stxxl::default_completion_handler());
        req->wait();
        double end = stxxl_timestamp();

        STXXL_MSG("Pos: " << pos << " block_size:" << block_size << " time:" << (end - begin));
        pos += 1024 * 1024 * 1024;
    }

    double sum = 0.;
    double sum2 = 0.;
    STXXL_MSG("Random write");
    const unsigned int times = 80;
    for (i = 0; i < times; i++)
    {
        random_number<> rnd;
        pos = rnd(disk_size / block_size) * block_size;
        double begin = stxxl_timestamp();
        req = file1.awrite(buffer, pos, block_size,stxxl::default_completion_handler());
        req->wait();
        double diff = stxxl_timestamp() - begin;

        sum += diff;
        sum2 += diff * diff;

        STXXL_MSG("Pos: " << pos << " block_size:" << block_size << " time:" << (diff));
    };

    sum = sum / double (times);
    sum2 = sum2 / double (times);
    assert(sum2 - sum * sum >= 0.0 );
    double err = sqrt(sum2 - sum * sum);
    STXXL_MSG("Error: " << err << " s, " << 100. * (err / sum) << " %");

    delete [] buffer;

    return 0;
}


