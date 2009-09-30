/***************************************************************************
 *  include/stxxl/bits/mng/config.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
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

#include <stxxl/bits/singleton.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/bits/common/utils.h>


__STXXL_BEGIN_NAMESPACE

//! \ingroup mnglayer

//! \brief Access point to disks properties
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
    };
    std::vector<DiskEntry> disks_props;

    // in disks_props, flash devices come after all regular disks
    unsigned first_flash;

    config()
    {
        const char * cfg_path = getenv("STXXLCFG");
        if (cfg_path)
            init(cfg_path);
        else
            init();
    }

    ~config()
    {
        for (unsigned i = 0; i < disks_props.size(); ++i) {
            if (disks_props[i].delete_on_exit) {
                STXXL_ERRMSG("Removing disk file created from default configuration: " << disks_props[i].path);
                unlink(disks_props[i].path.c_str());
            }
        }
    }

    void init(const char * config_path = "./.stxxl");

public:
    //! \brief Returns number of disks available to user
    //! \return number of disks
    inline unsigned disks_number() const
    {
        return disks_props.size();
    }

    //! \brief Returns contiguous range of regular disks w/o flash devices in the array of all disks
    //! \return range [begin, end) of regular disk indices
    inline std::pair<unsigned, unsigned> regular_disk_range() const
    {
        return std::pair<unsigned, unsigned>(0, first_flash);
    }

    //! \brief Returns contiguous range of flash devices in the array of all disks
    //! \return range [begin, end) of flash device indices
    inline std::pair<unsigned, unsigned> flash_range() const
    {
        return std::pair<unsigned, unsigned>(first_flash, disks_props.size());
    }

    //! \brief Returns path of disks
    //! \param disk disk's identifier
    //! \return string that contains the disk's path name
    inline const std::string & disk_path(int disk) const
    {
        return disks_props[disk].path;
    }
    //! \brief Returns disk size
    //! \param disk disk's identifier
    //! \return disk size in bytes
    inline stxxl::int64 disk_size(int disk) const
    {
        return disks_props[disk].size;
    }
    //! \brief Returns name of I/O implementation of particular disk
    //! \param disk disk's identifier
    inline const std::string & disk_io_impl(int disk) const
    {
        return disks_props[disk].io_impl;
    }
};

__STXXL_END_NAMESPACE


#endif // !STXXL_MNG__CONFIG_H
// vim: et:ts=4:sw=4
