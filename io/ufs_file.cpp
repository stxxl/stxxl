#include "stxxl/bits/io/ufs_file.h"

#ifndef BOOST_MSVC
 #include <unistd.h>
 #include <fcntl.h>
#endif

__STXXL_BEGIN_NAMESPACE


int ufs_file_base::get_file_des() const
{
    return file_des;
}

void ufs_file_base::lock()
{
#ifdef BOOST_MSVC
    // not yet implemented
#else
    struct flock lock_struct;
    lock_struct.l_type = F_RDLCK | F_WRLCK;
    lock_struct.l_whence = SEEK_SET;
    lock_struct.l_start = 0;
    lock_struct.l_len = 0; // lock all bytes
    stxxl_ifcheck_i ((::fcntl (file_des, F_SETLK, &lock_struct)),
                     "Filedescriptor=" << file_des, io_error)
#endif
}


ufs_request_base::ufs_request_base (
    ufs_file_base * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t,
    completion_handler on_cmpl) :
    request (on_cmpl, f, buf, off, b, t),
    /*		file (f),
                    buffer (buf),
                    offset (off),
                    bytes (b),
                    type(t), */
    _state (OP)
{
#ifdef STXXL_CHECK_BLOCK_ALIGNING
    // Direct I/O requires file system block size alignment for file offsets,
    // memory buffer addresses, and transfer(buffer) size must be multiple
    // of the file system block size
    check_aligning ();
#endif
}

bool ufs_request_base::add_waiter (onoff_switch * sw)
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(waiters_mutex);
#else
    waiters_mutex.lock ();
#endif

    if (poll ())                     // request already finished
    {
#ifndef STXXL_BOOST_THREADS
        waiters_mutex.unlock ();
#endif
        return true;
    }

    waiters.insert (sw);
#ifndef STXXL_BOOST_THREADS
    waiters_mutex.unlock ();
#endif

    return false;
}

void ufs_request_base::delete_waiter (onoff_switch * sw)
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(waiters_mutex);
    waiters.erase (sw);
#else
    waiters_mutex.lock ();
    waiters.erase (sw);
    waiters_mutex.unlock ();
#endif
}
int ufs_request_base::nwaiters ()                 // returns number of waiters
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(waiters_mutex);
    return waiters.size();
#else
    waiters_mutex.lock ();
    int size = waiters.size ();
    waiters_mutex.unlock ();
    return size;
#endif
}

void ufs_request_base::check_aligning ()
{
    if (offset % BLOCK_ALIGN != 0)
        STXXL_ERRMSG ("Offset is not aligned: modulo "
                                              << BLOCK_ALIGN << " = " <<
                      offset % BLOCK_ALIGN);

    if (bytes % BLOCK_ALIGN != 0)
        STXXL_ERRMSG ("Size is not a multiple of " <<
                      BLOCK_ALIGN << ", = " << bytes % BLOCK_ALIGN);

    if (long (buffer) % BLOCK_ALIGN != 0)
        STXXL_ERRMSG ("Buffer is not aligned: modulo "
                                              << BLOCK_ALIGN << " = " <<
                      long (buffer) % BLOCK_ALIGN << " (" <<
                      std::hex << buffer << std::dec << ")");
}

ufs_request_base::~ufs_request_base ()
{
    STXXL_VERBOSE3("ufs_request_base " << unsigned (this) << ": deletion, cnt: " << ref_cnt);

    assert(_state() == DONE || _state() == READY2DIE);

    // if(_state() != DONE && _state()!= READY2DIE )
    //	STXXL_ERRMSG("WARNING: serious stxxl error request being deleted while I/O did not finish "<<
    //		"! Please report it to the stxxl author(s) <dementiev@mpi-sb.mpg.de>");

    // _state.wait_for (READY2DIE); // does not make sense ?
}

void ufs_request_base::wait ()
{
    STXXL_VERBOSE3("ufs_request_base : " << unsigned (this) << " wait ");

    START_COUNT_WAIT_TIME

#ifdef NO_OVERLAPPING
    enqueue();
#endif

    _state.wait_for (READY2DIE);

    END_COUNT_WAIT_TIME

    check_errors();
}
bool ufs_request_base::poll()
{
#ifdef NO_OVERLAPPING
    /*if(_state () < DONE)*/ wait();
#endif

    bool s = _state() >= DONE;

    check_errors();

    return s;
}
const char * ufs_request_base::io_type ()
{
    return "ufs_base";
}

ufs_file_base::ufs_file_base (
    const std::string & filename,
    int mode,
    int disk) : file (disk), file_des (-1), mode_(mode)
{
    int fmode = 0;
#ifndef STXXL_DIRECT_IO_OFF
 #ifndef BOOST_MSVC
    if (mode & DIRECT)
        fmode |= O_SYNC | O_RSYNC | O_DSYNC | O_DIRECT;

 #endif
#endif
    if (mode & RDONLY)
        fmode |= O_RDONLY;

    if (mode & WRONLY)
        fmode |= O_WRONLY;

    if (mode & RDWR)
        fmode |= O_RDWR;

    if (mode & CREAT)
        fmode |= O_CREAT;

    if (mode & TRUNC)
        fmode |= O_TRUNC;


#ifdef BOOST_MSVC
    fmode |= O_BINARY;                     // the default in MS is TEXT mode
#endif


#ifdef BOOST_MSVC
    stxxl_ifcheck_i ((file_des = ::open (filename.c_str(), fmode,
                                         S_IREAD | S_IWRITE )),
                     "Filedescriptor=" << file_des << " filename=" << filename << " fmode=" << fmode, io_error);
#else
    stxxl_ifcheck_i ((file_des = ::open (filename.c_str(), fmode,
                                         S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP)),
                     "Filedescriptor=" << file_des << " filename=" << filename << " fmode=" << fmode, io_error)
#endif
}
ufs_file_base::~ufs_file_base ()
{
    int res = ::close (file_des);

    // if successful, reset file descriptor
    if (res >= 0)
        file_des = -1;

    else
        stxxl_function_error(io_error);
}
stxxl::int64 ufs_file_base::size ()
{
    struct stat st;
    stxxl_check_ge_0(fstat(file_des, &st), io_error);
    return st.st_size;
}
void ufs_file_base::set_size (stxxl::int64 newsize)
{
    stxxl::int64 cur_size = size();

#ifdef BOOST_MSVC
    // FIXME: ADD TRUNCATION HERE, CURRENTLY NO SUITABLE FUNCTION FOUND
#else
    if (!(mode_ & RDONLY))
        stxxl_check_ge_0(::ftruncate(file_des, newsize), io_error);

#endif

    if (newsize > cur_size)
        stxxl_check_ge_0(::lseek(file_des, newsize - 1, SEEK_SET), io_error);
}


__STXXL_END_NAMESPACE


