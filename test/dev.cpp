/***************************************************************************
 *            dev.cpp
 *
 *  Sat Aug 24 23:52:42 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <ustat.h>

#ifdef __linux
#include <sys/vfs.h>
#else
#include <sys/statvfs.h>
#endif

#include "../common/myutils.h"

int
main (int argc, char *argv[])
{
	if (argc < 2)
	{
		cerr << argv[0] << " path " << endl;
		exit (-1);
	}

	struct stat st;
	myifcheck (stat (argv[1], &st));
	cout << "Device(" << st.st_dev << ") major: " << major (st.
								st_dev) <<
		" minor: " << minor (st.st_dev) << endl;

	struct ustat ust;
	myifcheck (ustat (st.st_dev, &ust));

	cout << "Total free blocks: " << ust.f_tfree << endl;
	cout << "Number of free inodes: " << ust.f_tinode << endl;
	cout << "Filsys name: " << ust.f_fname << endl;
	cout << "Filsys pack name: " << ust.f_fpack << endl;

#ifdef __linux
	struct statfs stfs;
	myifcheck (statfs (argv[1], &stfs));

	cout << "type of filesystem: " << hex << stfs.f_type << dec << endl;
	cout << "optimal transfer block size: " << stfs.f_blocks << endl;
	cout << "file system id: " << stfs.f_fsid.__val[0] << " " << stfs.
		f_fsid.__val[1] << endl;
#else
	struct statvfs stfs;
	myifcheck (statvfs (argv[1], &stfs));

	cout << "preferred file system block size: " << stfs.f_bsize << endl;
	cout << "fundamental filesystem block: " << stfs.f_frsize << endl;
	cout << "file system id (dev for now:) " << stfs.f_fsid << endl;
	cout << "target fs type name: " << stfs.f_basetype << endl;
	cout << "file system specific string: " << stfs.f_fstr << endl;

#endif

	return 0;
}
