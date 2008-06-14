/***************************************************************************
 *            test_allocstr.cpp
 *
 *  instantiate all allocation strategies
 *
 *  Copyright © 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 ****************************************************************************/

#include <stxxl/mng>

template <typename strategy>
void test_strategy()
{
    STXXL_MSG(STXXL_PRETTY_FUNCTION_NAME);
    strategy s0;
    strategy s2(1, 3);
    stxxl::offset_allocator<strategy> o(1, s0);
    for (int i = 0; i < 16; ++i)
        STXXL_MSG(s0(i) << " " << s2(i) << " " << o(i));
}

int main ()
{
    stxxl::config * cfg = stxxl::config::get_instance();

    // instantiate the allocation strategies
    STXXL_MSG("Number of disks: " << cfg->disks_number());
    for (unsigned i = 0; i < cfg->disks_number(); ++i)
        STXXL_MSG(cfg->disk_path(i) << ", " << cfg->disk_size(i) << ", " << cfg->disk_io_impl(i));
    test_strategy<stxxl::striping>();
    test_strategy<stxxl::FR>();
    test_strategy<stxxl::SR>();
    test_strategy<stxxl::RC>();
    STXXL_MSG("Regular disks: [" << cfg->regular_disk_range().first << "," << cfg->regular_disk_range().second << ")");
    if (cfg->regular_disk_range().first != cfg->regular_disk_range().second)
        test_strategy<stxxl::RC_disk>();
    STXXL_MSG("Flash devices: [" << cfg->flash_range().first << "," << cfg->flash_range().second << ")");
    if (cfg->flash_range().first != cfg->flash_range().second)
        test_strategy<stxxl::RC_flash>();
    test_strategy<stxxl::single_disk>();
}
