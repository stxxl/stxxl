/***************************************************************************
 *  include/stxxl/bits/io/fileperblock_file.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2008, 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_FILEPERBLOCK_FILE_HEADER
#define STXXL_IO_FILEPERBLOCK_FILE_HEADER

#include <stxxl/bits/io/disk_queued_file.h>

#include <string>

namespace stxxl {

//! \addtogroup fileimpl
//! \{

//! Implementation of file based on other files, dynamically allocate one file per block.
//! Allows for dynamic disk space consumption.
template <class base_file_type>
class fileperblock_file : public disk_queued_file
{
private:
    std::string filename_prefix_;
    int mode_;
    offset_type current_size_;
    counting_ptr<base_file_type> lock_file_;

protected:
    //! Constructs a file name for a given block.
    std::string filename_for_block(offset_type offset);

public:
    //! Constructs file object.
    //! param filename_prefix_  filename prefix, numbering will be appended to it
    //! param mode_ open mode_, see \c file::open_modes
    fileperblock_file(
        const std::string& filename_prefix,
        int mode,
        int queue_id = DEFAULT_QUEUE,
        int allocator_id = NO_ALLOCATOR,
        unsigned int device_id = DEFAULT_DEVICE_ID);

    virtual ~fileperblock_file();

    void serve(void* buffer, offset_type offset, size_type bytes,
               request::read_or_write op) final;

    //! Changes the size of the file.
    //! \param new_size value of the new file size
    virtual void set_size(offset_type new_size) { current_size_ = new_size; }

    //! Returns size of the file.
    //! \return file size in length
    virtual offset_type size() { return current_size_; }

    virtual void lock();

    //! Frees the specified region.
    //! Actually deletes the corresponding file if the whole thing is deleted.
    virtual void discard(offset_type offset, offset_type length);

    //! Rename the file corresponding to the offset such that it is out of reach for deleting.
    virtual void export_files(offset_type offset, offset_type length, std::string filename);

    const char * io_type() const final;
};

//! \}

} // namespace stxxl

#endif // !STXXL_IO_FILEPERBLOCK_FILE_HEADER
