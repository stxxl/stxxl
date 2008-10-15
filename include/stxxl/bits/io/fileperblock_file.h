/***************************************************************************
 *  include/stxxl/bits/io/fileperblock_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_FILEPERBLOCK_FILE_HEADER
#define STXXL_FILEPERBLOCK_FILE_HEADER

#include <string>
#include <stxxl/bits/io/ufs_file.h>
#include <stxxl/bits/common/debug.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

template<class base_file_type>
class fileperblock_request;

//! \brief Implementation of file based on other files, dynamically allocated one per block
template<class base_file_type>
class fileperblock_file : public file
{
    friend class fileperblock_request<base_file_type>;

private:
    std::string filename;
    int mode;
    unsigned_type block_size;
    int disk;
    unsigned_type size_;

protected:
    std::string file_name_for_block(unsigned_type);

public:
    //! \brief constructs file object
    //! \param filename path of file(s), will append to it
    //! \param mode open mode, see \c file::open_modes
    //! \param disk disk(file) identifier
    fileperblock_file(
        const std::string & filename,
        int mode,
        unsigned_type block_size,
        int disk = -1);
    virtual request_ptr aread(
        void * buffer,
        int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    virtual request_ptr awrite(
        void * buffer,
        int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    //! \brief Changes the size of the file
    //! \param newsize value of the new file size
    virtual void set_size(int64 newsize) { size_ = newsize; }
    //! \brief Returns size of the file
    //! \return file size in bytes
    virtual int64 size() { return size_;}

    virtual void delete_region(int64 offset, unsigned_type size);

    void feasible(int64 pos, size_t bytes);
    int64 block_offset(int64 pos);
};

//! \brief Implementation of request based on UNIX fileperblocks
template<class base_file_type>
class fileperblock_request : public request
{
    friend class fileperblock_file<base_file_type>;

    typedef typename base_file_type::request base_request_type;

    base_file_type* base_file;
    request_ptr base_request;

protected:
    fileperblock_request(
        fileperblock_file<base_file_type>* f,
        void * buf,
        int64 off,
        size_t b,
        typename base_request_type::request_type t,
        completion_handler on_cmpl);
    ~fileperblock_request();
    virtual void serve();

public:
    virtual const char * io_type();

    virtual bool add_waiter(onoff_switch * sw);
    virtual void delete_waiter(onoff_switch * sw);
    virtual void wait();
    virtual bool poll();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_FILEPERBLOCK_FILE_HEADER
