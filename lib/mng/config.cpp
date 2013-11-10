/***************************************************************************
 *  lib/mng/config.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <fstream>
#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/mng/mng.h>
#include <stxxl/version.h>

#if STXXL_WINDOWS
   #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#else
  #include <unistd.h>
#endif

__STXXL_BEGIN_NAMESPACE

static inline bool exist_file(const std::string& path)
{
    //STXXL_MSG("Checking " << path << " for disk configuration.");
    std::ifstream in(path.c_str());
    return in.good();
}

void config::init_findconfig()
{
    // check different locations for disk configuration files

    // check STXXLCFG environment path
    const char* stxxlcfg = getenv("STXXLCFG");
    if (stxxlcfg && exist_file(stxxlcfg)) {
        init(stxxlcfg);
        return;
    }

#if !STXXL_WINDOWS
    // read environment, unix style
    const char* hostname = getenv("HOSTNAME");
    const char* home = getenv("HOME");
    const char* suffix = "";
#else
    // read environment, windows style
    const char* hostname = getenv("COMPUTERNAME");
    const char* home = getenv("APPDATA");
    const char* suffix = ".txt";
#endif

    // check current directory
    {
        std::string basepath = "./.stxxl";

        if (hostname && exist_file(basepath + "." + hostname + suffix)) {
            init(basepath + "." + hostname + suffix);
            return;
        }

        if (exist_file(basepath + suffix)) {
            init(basepath + suffix);
            return;
        }
    }

    // check home directory
    if (home)
    {
        std::string basepath = std::string(home) + "/.stxxl";

        if (hostname && exist_file(basepath + "." + hostname + suffix)) {
            init(basepath + "." + hostname + suffix);
            return;
        }

        if (exist_file(basepath + suffix)) {
            init(basepath + suffix);
            return;
        }
    }

    // load default configuration
    init("");
}

config::~config()
{
    for (disk_list_type::const_iterator it = disks_list.begin();
         it != disks_list.end(); it++)
    {
        if (it->delete_on_exit)
        {
            STXXL_ERRMSG("Removing disk file: " << it->path);
            unlink(it->path.c_str());
        }
    }
}

void config::init(const std::string& config_path)
{
    std::vector<disk_config> flash_list;
    std::ifstream cfg_file(config_path.c_str());
    if (!cfg_file)
    {
        STXXL_ERRMSG("Warning: no config file found.");
        STXXL_ERRMSG("Using default disk configuration.");
#if !STXXL_WINDOWS
        disk_config entry1("/var/tmp/stxxl", 1000 * 1024 * 1024, "syscall");
        entry1.delete_on_exit = true;
        entry1.autogrow = true;
#else
        disk_config entry1("", 1000 * 1024 * 1024, "wincall");
        entry1.delete_on_exit = true;
        entry1.autogrow = true;

        char * tmpstr = new char[255];
        if (GetTempPath(255, tmpstr) == 0)
            STXXL_THROW_WIN_LASTERROR(resource_error, "GetTempPath()");
        entry1.path = tmpstr;
        entry1.path += "stxxl.tmp";
        delete[] tmpstr;
#endif
        disks_list.push_back(entry1);
    }
    else
    {
        std::string line;

        while ( std::getline(cfg_file, line) )
        {
            // skip comments
            if (line.size() == 0 || line[0] == '#') continue;

            disk_config entry;
            entry.parseline(line); // throws on errors

            if (!entry.flash)
                disks_list.push_back(entry);
            else
                flash_list.push_back(entry);
        }
        cfg_file.close();
    }

    // put flash devices after regular disks
    first_flash = (unsigned int)disks_list.size();
    disks_list.insert(disks_list.end(), flash_list.begin(), flash_list.end());

    if (disks_list.empty()) {
        STXXL_THROW(std::runtime_error,
                    "No disks found in '" << config_path << "'.");
    }

    int64 total_size = 0;
    for (disk_list_type::const_iterator it = disks_list.begin();
         it != disks_list.end(); it++)
    {
        STXXL_MSG("Disk '" << it->path << "' is allocated, space: " <<
                  (it->size) / (1024 * 1024) <<
                  " MiB, I/O implementation: " << it->fileio_string());

        total_size += (*it).size;
    }

    if (disks_list.size() > 1)
    {
        STXXL_MSG("In total " << disks_list.size() << " disks are allocated, space: " <<
                  (total_size / (1024 * 1024)) <<
                  " MiB");
    }
}

disk_config::disk_config()
    : size(0),
      autogrow(false),
      delete_on_exit(false),
      direct(2),
      flash(false),
      queue(file::DEFAULT_QUEUE),
      unlink_on_open(false)
{
}

disk_config::disk_config(const std::string& _path, uint64 _size,
                         const std::string& _io_impl)
    : path(_path),
      size(_size),
      io_impl(_io_impl),
      autogrow(false),
      delete_on_exit(false),
      direct(2),
      flash(false),
      queue(file::DEFAULT_QUEUE),
      unlink_on_open(false)
{
}

void disk_config::parseline(const std::string& line)
{
    // split off disk= or flash=
    std::vector<std::string> eqfield = split(line, "=", 2, 2);

    if (eqfield[0] == "disk") {
        flash = false;
    }
    else if (eqfield[0] == "flash") {
        flash = true;
    }
    else {
        STXXL_THROW(std::runtime_error,
                    "Unknown configuration token " << eqfield[0]);
    }

    // *** Set Default Extra Options ***

    autogrow = false;
    delete_on_exit = false;
    direct = 2; // try DIRECT, otherwise fallback
    // flash is already set
    queue = file::DEFAULT_QUEUE;
    unlink_on_open = false;

    // *** Save Basic Options ***

    // split at commands, at least 3 fields
    std::vector<std::string> cmfield = split(eqfield[1], ",", 3, 3);

    // path:
    path = cmfield[0];
    // replace ### -> pid in path
    {
        std::string::size_type pos;
        if ( (pos = path.find("###")) != std::string::npos )
        {
#if !STXXL_WINDOWS
            int pid = getpid();
#else
            DWORD pid = GetCurrentProcessId();
#endif
            path.replace(pos, 3, to_str(pid));
        }
    }

    // size:
    if (!parse_SI_IEC_size(cmfield[1], size)) {
        STXXL_THROW(std::runtime_error,
                    "Invalid disk size '" << cmfield[1] << "' in disk configuration file.");
    }

    if (size == 0) {
        autogrow = true;
        delete_on_exit = true;
    }

    // io_impl:
    io_impl = cmfield[2];

    // split off extra fileio parameters
    size_t spacepos = io_impl.find(' ');
    if (spacepos == std::string::npos)
        return; // no space in fileio

    // *** Parse Extra Fileio Parameters ***

    std::string paramstr = io_impl.substr(spacepos+1);
    io_impl = io_impl.substr(0, spacepos);

    std::vector<std::string> param = split(paramstr, " ");

    for (std::vector<std::string>::const_iterator p = param.begin();
         p != param.end(); ++p)
    {
        // split at equal sign
        std::vector<std::string> eq = split(*p, "=", 2, 2);

        // *** PLEASE try to keep the elseifs sorted by parameter name!
        if (*p == "") {
            // skip blank options
        }
        else if (*p == "autogrow")
        {
            // TODO: which fileio implementation support autogrow?
            autogrow = true;
        }
        else if (*p == "delete" || *p == "delete_on_exit")
        {
            delete_on_exit = true;
        }
        else if (*p == "direct" || *p == "nodirect" || eq[0] == "direct")
        {
            // io_impl is not checked here, but I guess that is okay for DIRECT
            // since it depends highly platform _and_ build-time configuration.

            if (*p == "direct")         direct = 1; // force ON
            else if (*p == "nodirect")  direct = 0; // force OFF
            else if (eq[1] == "off")    direct = 0;
            else if (eq[1] == "on")     direct = 1;
            else if (eq[1] == "try")    direct = 2;
            else
            {
                STXXL_THROW(std::runtime_error,
                            "Invalid parameter '" << *p << "' in disk configuration file.");
            }
        }
        else if (eq[0] == "queue")
        {
            char* endp;
            queue = strtoul(eq[1].c_str(), &endp, 10);
            if (endp && *endp != 0) {
                STXXL_THROW(std::runtime_error,
                            "Invalid parameter '" << *p << "' in disk configuration file.");
            }
        }
        else if (*p == "unlink" || *p == "unlink_on_open")
        {
            if (!(io_impl == "syscall" || io_impl == "mmap") || io_impl == "wbtl") {
                STXXL_THROW(std::runtime_error,
                            "Parameter '" << *p << "' invalid for fileio '" << io_impl << "' in disk configuration file.");
            }

            unlink_on_open = true;
        }
        else
        {
            STXXL_THROW(std::runtime_error,
                        "Invalid optional parameter '" << *p << "' in disk configuration file.");
        }
    }

}

std::string disk_config::fileio_string() const
{
    std::ostringstream oss;

    oss << io_impl;

    if (autogrow)
        oss << " autogrow";

    if (delete_on_exit)
        oss << " delete_on_exit";

    // tristate direct variable: OFF, ON, TRY
    if (direct == 0)
        oss << " direct=off";
    else if (direct == 1)
        oss << " direct=on";
    else if (direct == 2)
        ; // silenced: oss << " direct=try";
    else
        assert(!"Invalid setting for 'direct' option.");

    if (flash)
        oss << " flash";

    if (queue != file::DEFAULT_QUEUE)
        oss << " queue=" << queue;

    if (unlink_on_open)
        oss << " unlink_on_open";

    return oss.str();
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
