/***************************************************************************
 *  mng/config.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <fstream>
#include <stxxl/bits/mng/mng.h>
#include <stxxl/version.h>
#include <stxxl/bits/common/log.h>
#include <stxxl/bits/common/error_handling.h>

#ifdef BOOST_MSVC
 #include <windows.h>
#endif

__STXXL_BEGIN_NAMESPACE

void config::init(const char * config_path)
{
    logger::get_instance();
    STXXL_MSG(get_version_string());
    std::vector<DiskEntry> flash_props;
    std::ifstream cfg_file(config_path);
    if (!cfg_file)
    {
        STXXL_ERRMSG("Warning: no config file found.");
        STXXL_ERRMSG("Using default disk configuration.");
#ifndef BOOST_MSVC
        DiskEntry entry1 = { "/var/tmp/stxxl", "syscall", 1000 * 1024 * 1024, true, false };
#else
        DiskEntry entry1 = { "", "wincall", 1000 * 1024 * 1024, true, false };
        char * tmpstr = new char[255];
        stxxl_check_ne_0(GetTempPath(255, tmpstr), resource_error);
        entry1.path = tmpstr;
        entry1.path += "stxxl";
        delete[] tmpstr;
#endif
#if 0
        DiskEntry entry2 =
        { "/tmp/stxxl1", "mmap", 100 * 1024 * 1024, true };
        DiskEntry entry3 =
        { "/tmp/stxxl2", "simdisk", 1000 * 1024 * 1024, false };
#endif
        disks_props.push_back(entry1);
        //disks_props.push_back(entry2);
        //disks_props.push_back(entry3);
    }
    else
    {
        std::string line;

        while (cfg_file >> line)
        {
            std::vector<std::string> tmp = split(line, "=");
            bool is_disk;

            if (tmp[0][0] == '#')
            { }
            else if ((is_disk = (tmp[0] == "disk")) || tmp[0] == "flash")
            {
                tmp = split(tmp[1], ",");
                DiskEntry entry = {
                    tmp[0], tmp[2],
                    int64(atoi(tmp[1].c_str())) * int64(1024 * 1024),
                    false,
                    false
                };
                if (entry.size == 0)
                    entry.autogrow = true;
                if (is_disk)
                    disks_props.push_back(entry);
                else
                    flash_props.push_back(entry);
            }
            else
            {
                std::cerr << "Unknown token " <<
                tmp[0] << std::endl;
            }
        }
        cfg_file.close();
    }

    // put flash devices after regular disks
    first_flash = disks_props.size();
    disks_props.insert(disks_props.end(), flash_props.begin(), flash_props.end());

    if (disks_props.empty())
        STXXL_THROW(std::runtime_error, "config::config", "No disks found in '" << config_path << "' .");

#ifdef STXXL_VERBOSE_DISKS
    for (std::vector<DiskEntry>::const_iterator it = disks_props.begin(); it != disks_props.end(); it++)
    {
        STXXL_MSG("Disk '" << (*it).path << "' is allocated, space: " <<
                  ((*it).size) / (1024 * 1024) <<
                  " MiB, I/O implementation: " << (*it).io_impl);
    }
#else
    int64 total_size = 0;
    for (std::vector<DiskEntry>::const_iterator it = disks_props.begin(); it != disks_props.end(); it++)
        total_size += (*it).size;

    STXXL_MSG("" << disks_props.size() << " disks are allocated, total space: " <<
              (total_size / (1024 * 1024)) <<
              " MiB");
#endif
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
