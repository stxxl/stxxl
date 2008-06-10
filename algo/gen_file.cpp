#include "stxxl/mng"
#include "stxxl/ksort"
#include "stxxl/vector"

using namespace stxxl;


struct my_type
{
    typedef unsigned key_type;

    key_type _key;
    char _data[128 - sizeof(key_type)];
    key_type key() const
    {
        return _key;
    }

    my_type() { }
    my_type(key_type __key) : _key(__key) { }

    static my_type min_value()
    {
        return my_type(0);
    }
    static my_type max_value()
    {
        return my_type(0xffffffff);
    }
};

bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}



int main()
{
    my_type::key_type max_key = 1 * 1024 * 1024;
    unsigned int block_size = 1 * 1024 * 1024;
    unsigned int records_in_block = block_size / sizeof(my_type);
    my_type * array = (my_type *) aligned_alloc<BLOCK_ALIGN>(block_size);
    syscall_file f("./in", file::CREAT | file::RDWR);
    request_ptr req;

    my_type::key_type cur_key = max_key;
    for (unsigned i = 0; i < max_key / records_in_block; i++ )
    {
        for (unsigned j = 0; j < records_in_block; j++)
            array[j]._key = cur_key--;


        req = f.awrite((void *)array, stxxl::int64(i) * block_size, block_size, stxxl::default_completion_handler());
        req->wait();
    }

    aligned_dealloc<BLOCK_ALIGN>(array);

    return 0;
}
