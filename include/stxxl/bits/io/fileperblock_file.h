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

//! \brief Implementation of file based on other files, dynamically allocate one file per block.
//! Allows for dynamic disk space consumption.
template<class base_file_type>
class fileperblock_file : public file
{
    friend class fileperblock_request<base_file_type>;

private:
    std::string filename_prefix;
    int mode;
    int disk;
    unsigned_type current_size;

protected:
    //! \brief Constructs a file name for a given block.
    std::string filename_for_block(unsigned_type offset);

    //! \brief Check whether the addresses are feasible in for this configuration.
    bool feasible(int64 offset, size_t length);

    //! \brief Computes the block-local offset.
    int64 block_offset(int64 offset);

public:
    //! \brief constructs file object
    //! \param filename_prefix  filename prefix, numbering will be appended to it
    //! \param mode open mode, see \c file::open_modes
    //! \param disk disk(file) identifier
    fileperblock_file(
        const std::string & filename_prefix,
        int mode,
        int disk = -1);

    //! \brief Schedules asynchronous read request to the file
    //! \param buffer pointer to memory buffer to read into
    //! \param offset starting file position to read
    //! \param length number of length to transfer
    //! \param on_completion I/O completion handler
    //! \return \c request_ptr object, that can be used to track the status of the operation
    virtual request_ptr aread(
        void * buffer,
        int64 offset,
        size_t length,
        completion_handler on_completion);

    //! \brief Schedules asynchronous write request to the file
    //! \param buffer pointer to memory buffer to write from
    //! \param offset starting file position to write
    //! \param length number of length to transfer
    //! \param on_completion I/O completion handler
    //! \return \c request_ptr object, that can be used to track the status of the operation
    virtual request_ptr awrite(
        void * buffer,
        int64 offset,
        size_t length,
        completion_handler on_completion);

    //! \brief Changes the size of the file
    //! \param new_size value of the new file size
    virtual void set_size(int64 new_size) { current_size = new_size; }

    //! \brief Returns size of the file
    //! \return file size in length
    virtual int64 size() { return current_size;}

    //! \brief Frees the specified region.
    //! Actually deletes the corresponding file if the whole thing is deleted.
    virtual void delete_region(int64 offset, unsigned_type length);

    virtual void lock();

    //! Rename the file corresponding to the offset such that it is out of reach for deleting.
    virtual void export_files(int64 offset, int64 length, std::string filename);
};

//! \brief Request type associated with fileperblock_file.
template<class base_file_type>
class fileperblock_request : public request
{
    friend class fileperblock_file<base_file_type>;

    typedef typename base_file_type::request base_request_type;

    base_file_type* base_file;
    request_ptr* base_request;

protected:
    //! Constructs the request.
    //! \param f virtual file to access the data from.
    //! \param buffer buffer to write/read the data to/from.
    //! \param offset offset in the virtual file.
    //! \param length number of bytes of the request.
    //! \param read_or_write read or write the block?
    //! \param on_completion handler to call on completion.
    fileperblock_request(
        fileperblock_file<base_file_type>* f,
        void * buffer,
        int64 offset,
        size_t length,
        typename base_request_type::request_type read_or_write,
        completion_handler on_completion);

    ~fileperblock_request();

    //! \brief Serve the request, i. e. actually bring in the data.
    //! Will block thread until the operation is finished.
    virtual void serve();

public:
    //! \brief Identifies the type of request I/O implementation
    //! \return pointer to null terminated string of characters, containing the name of I/O implementation
    virtual const char * io_type();

    //! \brief Add one waiter to this request.
    virtual bool add_waiter(onoff_switch * sw);

    //! \brief Remove one waiter from this request.
    virtual void delete_waiter(onoff_switch * sw);

    //! \brief Suspends calling thread until completion of the request
    virtual void wait();

    //! \brief Polls the status of the request.
    //! \return \c true if request is completed, otherwise \c false
    virtual bool poll();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_FILEPERBLOCK_FILE_HEADER
