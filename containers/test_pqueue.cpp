/***************************************************************************
 *            test_pqueue.cpp
 *
 *  Fri Jul  4 11:31:34 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include <limits>
#include <stxxl/priority_queue>

#define RECORD_SIZE 128

//! \example containers/test_pqueue.cpp
//! This is an example of how to use \c stxxl::PRIORITY_QUEUE_GENERATOR
//! and \c stxxl::priority_queue

struct my_type
{
    typedef int key_type;
    key_type key;
    char data[RECORD_SIZE - sizeof(key_type)];
    my_type() { }
    explicit my_type(key_type k) : key(k) { }
};

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
    o << obj.key;
    return o;
}

struct my_cmp // greater
{
    bool operator ()  (const my_type & a, const my_type & b) const
    {
        return a.key > b.key;
    }

    my_type min_value() const
    {
        return my_type((std::numeric_limits < my_type::key_type > ::max)());
    }
};

int main()
{
/*
       unsigned BufferSize1_ = 32, // equalize procedure call overheads etc.
       unsigned N_ = 512, // bandwidth
       unsigned IntKMAX_ = 64, // maximal arity for internal mergers
       unsigned IntLevels_ = 4,
       unsigned BlockSize_ = (2*1024*1024),
       unsigned ExtKMAX_ = 64, // maximal arity for external mergers
       unsigned ExtLevels_ = 2,
 */
    //typedef priority_queue<priority_queue_config<my_type,my_cmp,
    //  32,512,64,3,(4*1024),0x7fffffff,1> > pq_type;
    const unsigned volume = 1024 * 1024; // in KB
    typedef stxxl::PRIORITY_QUEUE_GENERATOR < my_type, my_cmp, 32 * 1024 * 1024, volume / sizeof(my_type) > gen;
    typedef gen::result pq_type;
    typedef pq_type::block_type block_type;

    STXXL_MSG("Block size: " << block_type::raw_size);
    STXXL_MSG("AI: " << gen::AI);
    STXXL_MSG("X : " << gen::X);
    STXXL_MSG("N : " << gen::N);
    STXXL_MSG("AE: " << gen::AE);

    stxxl::timer Timer;
    Timer.start();

    const unsigned mem_for_pools = 128 * 1024 * 1024;
    stxxl::prefetch_pool<block_type> p_pool((mem_for_pools / 2) / block_type::raw_size);
    stxxl::write_pool<block_type>    w_pool((mem_for_pools / 2) / block_type::raw_size);
    pq_type p(p_pool, w_pool);

    stxxl::int64 nelements = stxxl::int64(volume * 1024 / sizeof(my_type)), i;
    STXXL_MSG("Internal memory consumption of the priority queue: " << p.mem_cons() << " bytes");
    STXXL_MSG("Max elements: " << nelements);
    for (i = 0; i < nelements; i++ )
    {
        if ((i % (1024 * 1024)) == 0)
            STXXL_MSG("Inserting element " << i);
        p.push(my_type(nelements - i));
    }
    Timer.stop();
    STXXL_MSG("Time spent for filling: " << Timer.seconds() << " sec");

    // test swap
    pq_type p1(p_pool, w_pool);
    std::swap(p, p1);
    std::swap(p, p1);

    STXXL_MSG("Internal memory consumption of the priority queue: " << p.mem_cons() << " bytes");
    Timer.reset();
    Timer.start();
    for (i = 0; i < (nelements); ++i )
    {
        assert( !p.empty() );
        //STXXL_MSG( p.top() );
        assert(p.top().key == i + 1);
        p.pop();
        if ((i % (1024 * 1024)) == 0)
            STXXL_MSG("Element " << i << " popped");
    }
    Timer.stop();

    STXXL_MSG("Time spent for removing elements: " << Timer.seconds() << " sec");
    STXXL_MSG("Internal memory consumption of the priority queue: " << p.mem_cons() << " bytes");
}
