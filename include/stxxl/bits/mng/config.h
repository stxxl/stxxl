/***************************************************************************
 *  include/stxxl/bits/mng/config.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG__CONFIG_H
#define STXXL_MNG__CONFIG_H

#include <vector>
#include <string>
#include <cstdlib>
#include <unistd.h>

#include <stxxl/bits/singleton.h>
#include <stxxl/bits/verbose.h>


__STXXL_BEGIN_NAMESPACE

//! \ingroup mnglayer

//! Access point to disks properties.
//! \remarks is a singleton
class config : public singleton<config>
{
    friend class singleton<config>;

    struct DiskEntry
    {
        std::string path;
        std::string io_impl;
        stxxl::int64 size;
        bool delete_on_exit;
        bool autogrow;
    };

    std::vector<DiskEntry> disks_props;

    // in disks_props, flash devices come after all regular disks
    unsigned first_flash;

    //! searchs different locations for a disk configuration file
    config();

    ~config()
    {
        for (unsigned i = 0; i < disks_props.size(); ++i) {
            if (disks_props[i].delete_on_exit || disks_props[i].autogrow) {
                if (!disks_props[i].autogrow) {
                    STXXL_ERRMSG("Removing disk file created from default configuration: " << disks_props[i].path);
                }
                unlink(disks_props[i].path.c_str());
            }
        }
    }

    //! load disk configuration file
    void init(const std::string& config_path = "./.stxxl");

public:
    //! Returns number of disks available to user.
    //! \return number of disks
    inline unsigned disks_number() const
    {
        return disks_props.size();
    }

    //! Returns contiguous range of regular disks w/o flash devices in the array of all disks.
    //! \return range [begin, end) of regular disk indices
    inline std::pair<unsigned, unsigned> regular_disk_range() const
    {
        return std::pair<unsigned, unsigned>(0, first_flash);
    }

    //! Returns contiguous range of flash devices in the array of all disks.
    //! \return range [begin, end) of flash device indices
    inline std::pair<unsigned, unsigned> flash_range() const
    {
        return std::pair<unsigned, unsigned>(first_flash, (unsigned)disks_props.size());
    }

    //! Returns path of disks.
    //! \param disk disk's identifier
    //! \return string that contains the disk's path name
    inline const std::string & disk_path(int disk) const
    {
        return disks_props[disk].path;
    }

    //! Returns disk size.
    //! \param disk disk's identifier
    //! \return disk size in bytes
    inline stxxl::int64 disk_size(int disk) const
    {
        return disks_props[disk].size;
    }

    //! Returns name of I/O implementation of particular disk.
    //! \param disk disk's identifier
    inline const std::string & disk_io_impl(int disk) const
    {
        return disks_props[disk].io_impl;
    }
};

__STXXL_END_NAMESPACE

#endif // !STXXL_MNG__CONFIG_H
// vim: et:ts=4:sw=4
