#include <cassert>
#include <ctime>
#include <unistd.h>

#include "stxxl/bits/common/seed.h"
#include "stxxl/bits/common/mutex.h"


__STXXL_BEGIN_NAMESPACE

inline unsigned initial_seed();

struct seed_generator_t {
    unsigned seed;
    mutex mtx;

    seed_generator_t(unsigned s) : seed(s)
    { }
};

seed_generator_t & seed_generator() {
	static seed_generator_t sg(initial_seed());
	return sg;
}

//FIXME: Probably a different implementation of initial_seed() 
//       is needed for Windows. But please use something with a finer
//       resolution than time(NULL) (whole seconds only).

inline unsigned initial_seed()
{
    static bool initialized = false;
    assert(!initialized); // this should only be called once!

    initialized = true;
    struct timeval tv;
    gettimeofday(&tv, 0);

    return tv.tv_sec ^ tv.tv_usec ^ (getpid() << 16);
}

void set_seed(unsigned seed)
{
    seed_generator().mtx.lock();
    seed_generator().seed = seed;
    seed_generator().mtx.unlock();
}

unsigned get_next_seed()
{
    seed_generator().mtx.lock();
    unsigned seed = seed_generator().seed++;
    seed_generator().mtx.unlock();
    return seed;
}

__STXXL_END_NAMESPACE
