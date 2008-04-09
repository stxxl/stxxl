#ifndef SYSCALL_HEADER
#define SYSCALL_HEADER

/***************************************************************************
 *            syscall_file.h
 *
 *  Sat Aug 24 23:55:08 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/io/ufs_file.h"
#include "stxxl/bits/common/debug.h"


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on UNIX syscalls
class syscall_file : public ufs_file_base
{
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    syscall_file(
        const std::string & filename,
        int mode,
        int disk = -1);
    request_ptr aread(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    request_ptr awrite(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
};

//! \brief Implementation of request based on UNIX syscalls
class syscall_request : public ufs_request_base
{
    friend class syscall_file;
protected:
    syscall_request(
        syscall_file * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t,
        completion_handler on_cmpl);
    void serve ();
public:
    const char * io_type ();
};


//! \}

__STXXL_END_NAMESPACE


#endif
