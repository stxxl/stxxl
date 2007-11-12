/*! \mainpage Documentation for STXXL library
 *
 *  \image html logo1.png
 *
 * <BR><BR>
 * The core of \c S<small>TXXL</small> is an implementation of the C++
 * standard template library STL for external memory (out-of-core)
 * computations, i.e., \c S<small>TXXL</small> implements containers and algorithms
 * that can process huge volumes of data that only fit on
 * disks. While the compatibility to the STL supports
 * ease of use and compatibility with existing applications,
 * another design priority is high performance.
 * Here is a selection of \c S<small>TXXL</small> performance features:
 * - transparent support of multiple disks
 * - variable block lengths
 * - overlapping of I/O and computation
 * - prevention of OS file buffering overhead
 * - algorithm pipelining
 *
 * \section platforms Supported Operating Systems
 * - Linux (kernel >= 2.4.18)
 * - Solaris
 * - other POSIX compatible systems should work, but have not been tested
 * - Windows XP/2000
 *
 * \section compilers Supported Compilers
 *
 * The following compilers have been tested in different
 * \c S<small>TXXL</small> configurations.
 * Other compilers might work, too, but we don't have the resources
 * (systems, compilers or time) to test them.
 * Feedback is welcome.
 *
 * \verbatim
compiler      |  stxxl   stxxl     stxxl     stxxl
              |          + mcstl   + boost   + mcstl
              |                              + boost
--------------+----------------------------------------
GCC 4.3       |    x       x         -²        -²
GCC 4.2       |    x       x         x         x
GCC 4.1       |    x      n/a        ?        n/a
GCC 4.0       |    x      n/a        ?        n/a
GCC 3.4       |    x      n/a        ?        n/a
GCC 3.3       |    o      n/a        ?        n/a
GCC 2.95      |    -      n/a        -        n/a
ICPC 9.1.051  |    x       x¹        ?         ?
ICPC 10.0.025 |    x       x¹        ?         ?
MSVC 2005 8.0 |    x      n/a        x        n/a

 x   = full support
 o   = partial support
 -   = unsupported
 ?   = untested
 n/a = compiler does not support OpenMP which is needed by MCSTL
 ¹   = does not work with STL GCC 4.2.0 (ICPC bug), workaround:
       the first include in the program must be
       "stxxl/bits/common/intel_compatibility.h"
 ²   = Boost currently does not support g++ 4.3
\endverbatim
 *
 *
 * \section installation Installation and Usage Instructions
 *
 * - \link installation_linux_gcc Installation (Linux/g++) \endlink
 * - \link installation_solaris_gcc Installation (Solaris/g++) \endlink
 * - \link installation_msvc Installation (Windows/MS Visual C++ 7.1) \endlink
 * - \link installation_old Installation of the older Stxxl versions (earlier than 0.9) (Linux/g++) \endlink
 *
 * Questions concerning use and development of the \c S<small>TXXL</small>
 * library and bug reports should be posted to the
 * <b><a href="http://sourceforge.net/forum/?group_id=131632">FORUMS</a></b>
 * or mailed to <A href="http://i10www.ira.uka.de/dementiev/">Roman Dementiev</A>.
 *
 */


/*!
 * \page installation Installation
 * - \link installation_linux_gcc Installation (Linux/g++) \endlink
 * - \link installation_solaris_gcc Installation (Solaris/g++) \endlink
 * - \link installation_msvc Installation (Windows/MS Visual C++ 7.1) \endlink
 * - \link installation_old Installation of the older Stxxl versions (earlier than 0.9) (Linux/g++) \endlink
 */


/*!
 * \page installation_linux_gcc Installation (Linux/g++ - Stxxl from version 1.1)
 *
 * \section download Download and library compilation
 *
 * - Download the latest gzipped tarball from
 *   <A href="http://sourceforge.net/project/showfiles.php?group_id=131632&package_id=144407">SourceForge</A>.
 * - Unpack in some directory executing: \c tar \c zfxv \c stxxl-x.y.z.tgz ,
 * - Change to \c stxxl directory: \c cd \c stxxl-x.y.z ,
 * - Change \c make.settings.gnu or \c make.settings.local file according to your system configuration:
 *   - \c S<small>TXXL</small> root directory \c STXXL_ROOT variable
 *     ( \c directory_where_you_unpacked_the_tar_ball/stxxl-x.y.z )
 *   - if you want \c S<small>TXXL</small> to use <A href="http://www.boost.org">Boost</A> libraries
 *     (you should have the Boost libraries already installed)
 *     - change \c USE_BOOST variable to \c yes
 *     - change \c BOOST_ROOT variable according to the Boost root path
 *   - if you want \c S<small>TXXL</small> to use the <A href="http://algo2.iti.uni-karlsruhe.de/singler/mcstl/">MCSTL</A>
 *     library (you should have the MCSTL library already installed)
 *     - change \c MCSTL_ROOT variable according to the MCSTL root path
 *     - use the targets \c library_g++_mcstl and \c tests_g++_mcstl
 *       instead of the ones listed below
 *   - (optionally) set \c OPT variable to \c -O3 or other g++ optimization level you like
 *   - (optionally) set \c DEBUG variable to \c -g or other g++ debugging option
 *     if you want to produce a debug version of the Stxxl library or Stxxl examples
 * - Verify that you have GNU make 3.80 (or later) installed
 * - Run: \verbatim make library_g++ \endverbatim
 * - Run: \verbatim make tests_g++ \endverbatim (optional, if you want to compile and run some test programs)
 *
 *
 * \section compile_apps Application compilation
 *
 * After compiling the library, some Makefile variables are written to
 * \c stxxl.mk (\c mcstxxl.mk if you built with MCSTL) in your
 * \c STXXL_ROOT directory. This file should be included from your
 * application's Makefile.
 *
 * The following variables can be used:
 * - \c STXXL_CXX - the compiler used to build the \c S<small>TXXL</small>
 *      library, it's recommended to use the same to build your applications
 * - \c STXXL_CPPFLAGS - add these flags to the compile commands
 * - \c STXXL_LDLIBS - add these libraries to the link commands
 *
 * An example Makefile for an application using \c S<small>TXXL</small>:
 * \verbatim
STXXL_ROOT      ?= .../stxxl
STXXL_CONFIG    ?= stxxl.mk
include $(STXXL_ROOT)/$(STXXL_CONFIG)

# use the variables from stxxl.mk
CXX              = $(STXXL_CXX)
CPPFLAGS        += $(STXXL_CPPFLAGS)

# add your own optimization, warning, debug, ... flags
# (these are *not* set in stxxl.mk)
CPPFLAGS        += -O3 -Wall -g -DFOO=BAR

# build your application
# (my_example.o is generated from my_example.cpp automatically)
my_example.bin: my_example.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) my_example.o -o $@ $(STXXL_LDLIBS)
\endverbatim
 *
 * Before you try to run one of the \c S<small>TXXL</small> examples
 * (or your own \c S<small>TXXL</small> program) you must configure the disk
 * space that will be used as external memory for the library. For instructions how to do that,
 * see the next section.
 *
 *
 * \section space Disk space
 *
 * To get best performance with \c S<small>TXXL</small> you should assign separate disks to it.
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your
 * external memory application will increase if you use more than one disk.
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * of about 50-75 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 *
 *
 * \section filesystem Recommended file system
 *
 * Our library take benefit of direct user memory - disk transfers (direct access) which avoids
 * superfluous copies.
 * We recommend to use the
 * \c XFS file system (<A href="http://oss.sgi.com/projects/xfs/">link</A>) that
 * gives good read and write performance for large files.
 * Note that file creation speed of \c XFS is slow, so that disk
 * files should be precreated.
 *
 * If the filesystems only use is to store one large \c S<small>TXXL</small> disk file,
 * we also recommend to add the following options to the \c mkfs.xfs command to gain maximum performance:
 * \verbatim -d agcount=1 -l size=512b \endverbatim
 *
 *
 * \section configuration Disk configuration file
 *
 * You must define the disk configuration for an
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside
 * in the same directory where you execute the program.
 * You can change the default file name for the configuration
 * file by setting the environment variable \c STXXLCFG .
 *
 * Each line of the configuration file describes a disk.
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file
 *   access methods. Each disk is represented as a file. If you have a disk that is mounted in Unix
 *   to the path /mnt/disk0/, then the correct value for the \c full_disk_filename would be
 *   \c /mnt/disk0/some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different
 *   file access implementations for POSIX systems, choose one of them:
 *   - \c syscall uses \c read and \c write system calls which perform disk transfers directly
 *     on user memory pages without superfluous copying (currently the fastest method)
 *   - \c mmap : performs disks transfers using \c mmap and \c munmap system calls
 *   - \c simdisk : simulates timings of the IBM IC35L080AVVA07 disk, full_disk_filename must point
 *     to a file on a RAM disk partition with sufficient space
 *
 * See also the example configuration file \c 'config_example' included in the tarball.
 *
 *
 * \section excreation Precreating external memory files
 *
 * In order to get the maximum performance one should precreate disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The precreation utility is included in the set of \c S<small>TXXL</small>
 * utilities ( \c utils/createdisks.bin ). Run this utility
 * for each disk you have defined in the disk configuration file:
 * \verbatim utils/createdisks.bin capacity full_disk_filename... \endverbatim
 *
 * */


/*!
 * \page installation_msvc Installation (Windows/MS Visual C++ 7.1 - Stxxl from version 0.9)
 *
 * \section download Download and library compilation
 *
 * - Install the <a href="http://www.boost.org">Boost</a> libraries (required).
 * - Download the latest \c Stxxl zip file from
 *   <A href="http://sourceforge.net/project/showfiles.php?group_id=131632&package_id=144407">SourceForge</A>.
 * - Unpack the zip file in some directory (e.g. \c 'c:\\' ),
 * - Change to \c stxxl directory: \c cd \c stxxl-x.y.z ,
 * - Change \c make.settings.msvc file according to your system configuration:
 *   - \c S<small>TXXL</small> root directory \c STXXL_ROOT variable
 *     ( \c directory_where_you_unpacked_the_zipfile\\stxxl-x.y.z , e.g. \c 'c:\\stxxl' )
 *   - change \c BOOST_ROOT variable according to the Boost root path
 *   - (optionally) set \c OPT variable to \c /O2 or other VC++ optimization level you like
 *   - set \c DEBUG variable to \c /MDd for debug version of the \c Stxxl library
 *     or to \c /MD for the version without debugging information in object files
 * - Open the \c stxxl.vcproj file (VS Solution Object) in Visual Studio .NET.
 *   The file is located in the \c STXXL_ROOT directory
 * - Press F7 to build the library.
 *   The library file (libstxxl.lib) should appear in \c STXXL_ROOT\\lib directory
 * - In the configuration manager ('Build' drop-down menu) choose 'Library and tests'
 *   as active solution configuration. Press OK.
 * - Press F7 to build \c stxxl test programs.
 *
 *
 * \section compile_apps Application compilation
 *
 * Programs using Stxxl can be compiled using options from \c compiler.options
 * file (in the \c STXXL_ROOT directory). The linking options for the VC++
 * linker you can find in \c linker.options file. In order to accomplish this
 * do the following:
 * - Open project property pages (menu Project->Properties)
 * - Choose C/C++->Command Line page.
 * - In the 'Additional Options' field insert the contents of the \c compiler.options file.
 * Make sure that the Runtime libraries/debug options (/MDd or /MD or /MT or /MTd) of
 * the \c Stxxl library (see above) do not conflict with the options of your project.
 * Use the same options in the \c Stxxl and your project.
 * - Choose Linker->Command Line page.
 * - In the 'Additional Options' field insert the contents of the \c linker.options file.
 *
 * <BR>
 * If you use make files you can
 * include \c make.settings file in your make files and use \c STXXL_COMPILER_OPTIONS and
 * \c STXXL_LINKER_OPTIONS variables, defined therein.
 *
 * For example: <BR>
 * \verbatim cl -c my_example.cpp $(STXXL_COMPILER_OPTIONS) \endverbatim <BR>
 * \verbatim link my_example.obj /out:my_example.exe $(STXXL_LINKER_OPTIONS) \endverbatim
 *
 * <BR>
 * The \c STXXL_ROOT\\test\\WinGUI directory contains an example MFC GUI project
 * that uses \c Stxxl. In order to compile it open the WinGUI.vcproj file in
 * Visual Studio .NET. Change if needed the Compiler and Linker Options of the project
 * (see above).
 *
 * Before you try to run one of the \c S<small>TXXL</small> examples
 * (or your own \c S<small>TXXL</small> program) you must configure the disk
 * space that will be used as external memory for the library. For instructions how to do that,
 * see the next section.
 *
 *
 * \section space Disk space
 *
 * To get best performance with \c S<small>TXXL</small> you should assign separate disks to it.
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your
 * external memory application will increase if you use more than one disk.
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * of about 50-75 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 *
 *
 * \section configuration Disk configuration file
 *
 * You must define the disk configuration for an
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside
 * in the same directory where you execute the program.
 * You can change the default file name for the configuration
 * file by setting the environment variable \c STXXLCFG .
 *
 * Each line of the configuration file describes a disk.
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file
 *   access methods. Each disk is represented as a file. If you have a disk called \c e:
 *   then the correct value for the \c full_disk_filename would be
 *   \c e:\\some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different
 *   file access implementations for WINDOWS, choose one of them:
 *   - \c syscall: uses \c read and \c write POSIX system calls (slow)
 *   - \c wincall: performs disks transfers using \c ReadFile and \c WriteFile WinAPI calls
 *     This method supports direct I/O that avoids superfluous copying of data pages
 *     in the Windows kernel. This is the best (and default) method in Stxxl for Windows.
 *
 * See also the example configuration file \c 'config_example_win' included in the archive.
 *
 *
 * \section excreation Precreating external memory files
 *
 * In order to get the maximum performance one should precreate disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The precreation utility is included in the set of \c S<small>TXXL</small>
 * utilities ( \c utils\\createdisks.exe ). Run this utility
 * for each disk you have defined in the disk configuration file:
 * \verbatim utils\createdisks.exe capacity full_disk_filename... \endverbatim
 *
 * */


/*!
 * \page installation_solaris_gcc Installation (Solaris/g++ - Stxxl from version 1.1)
 *
 * \section download Download and library compilation
 *
 * - Download the latest gzipped tarball from
 *   <A href="http://sourceforge.net/project/showfiles.php?group_id=131632&package_id=144407">SourceForge</A>.
 * - Unpack in some directory executing: \c tar \c zfxv \c stxxl-x.y.z.tgz ,
 * - Change to \c stxxl directory: \c cd \c stxxl-x.y.z ,
 * - Change \c make.settings.gnu or \c make.settings.local file according to your system configuration:
 *   - \c S<small>TXXL</small> root directory \c STXXL_ROOT variable
 *     ( \c directory_where_you_unpacked_the_tar_ball/stxxl-x.y.z )
 *   - if you want \c S<small>TXXL</small> to use <A href="http://www.boost.org">Boost</A> libraries
 *     (you should have the Boost libraries already installed)
 *     - change \c USE_BOOST variable to \c yes
 *     - change \c BOOST_ROOT variable according to the Boost root path
 *   - (optionally) set \c OPT variable to \c -O3 or other g++ optimization level you like
 *   - (optionally) set \c DEBUG variable to \c -g or other g++ debugging option
 *     if you want to produce a debug version of the Stxxl library or Stxxl examples
 * - Verify that you have GNU make 3.80 (or later) installed
 * - Run: \verbatim make library_g++ \endverbatim
 * - Run: \verbatim make tests_g++ \endverbatim (optional, if you want to compile and run some test programs)
 *
 *
 * \section compile_apps Application compilation
 *
 * After compiling the library, some Makefile variables are written to
 * \c stxxl.mk (\c mcstxxl.mk if you built with MCSTL) in your
 * \c STXXL_ROOT directory. This file should be included from your
 * application's Makefile.
 *
 * The following variables can be used:
 * - \c STXXL_CXX - the compiler used to build the \c S<small>TXXL</small>
 *      library, it's recommended to use the same to build your applications
 * - \c STXXL_CPPFLAGS - add these flags to the compile commands
 * - \c STXXL_LDLIBS - add these libraries to the link commands
 *
 * An example Makefile for an application using \c S<small>TXXL</small>:
 * \verbatim
STXXL_ROOT      ?= .../stxxl
STXXL_CONFIG    ?= stxxl.mk
include $(STXXL_ROOT)/$(STXXL_CONFIG)

# use the variables from stxxl.mk
CXX              = $(STXXL_CXX)
CPPFLAGS        += $(STXXL_CPPFLAGS)

# add your own optimization, warning, debug, ... flags
# (these are *not* set in stxxl.mk)
CPPFLAGS        += -O3 -Wall -g -DFOO=BAR

# build your application
# (my_example.o is generated from my_example.cpp automatically)
my_example.bin: my_example.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) my_example.o -o $@ $(STXXL_LDLIBS)
\endverbatim
 *
 * Before you try to run one of the \c S<small>TXXL</small> examples
 * (or your own \c S<small>TXXL</small> program) you must configure the disk
 * space that will be used as external memory for the library. For instructions how to do that,
 * see the next section.
 *
 *
 * \section space Disk space
 *
 * To get best performance with \c S<small>TXXL</small> you should assign separate disks to it.
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your
 * external memory application will increase if you use more than one disk.
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * of about 50-75 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 *
 *
 * \section filesystem Recommended file system
 *
 * FIXME: XFS on Solaris ???
 *
 * Our library take benefit of direct user memory - disk transfers (direct access) which avoids
 * superfluous copies.
 * We recommend to use the
 * \c XFS file system <A href="http://oss.sgi.com/projects/xfs/">link</A> that
 * gives good read and write performance for large files.
 * Note that file creation speed of \c XFS is slow, so that disk
 * files should be precreated.
 *
 *
 * \section configuration Disk configuration file
 *
 * You must define the disk configuration for an
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside
 * in the same directory where you execute the program.
 * You can change the default file name for the configuration
 * file by setting the environment variable \c STXXLCFG .
 *
 * Each line of the configuration file describes a disk.
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file
 *   access methods. Each disk is represented as a file. If you have a disk that is mounted in Unix
 *   to the path /mnt/disk0/, then the correct value for the \c full_disk_filename would be
 *   \c /mnt/disk0/some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different
 *   file access implementations for POSIX systems, choose one of them:
 *   - \c syscall uses \c read and \c write system calls which perform disk transfers directly
 *     on user memory pages without superfluous copying (currently the fastest method)
 *   - \c mmap : performs disks transfers using \c mmap and \c munmap system calls
 *   - \c simdisk : simulates timings of the IBM IC35L080AVVA07 disk, full_disk_filename must point
 *     to a file on a RAM disk partition with sufficient space
 *
 * See also the example configuration file \c 'config_example' included in the tarball.
 *
 *
 * \section excreation Precreating external memory files
 *
 * In order to get the maximum performance one should precreate disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The precreation utility is included in the set of \c S<small>TXXL</small>
 * utilities ( \c utils/createdisks.bin ). Run this utility
 * for each disk you have defined in the disk configuration file:
 * \verbatim utils/createdisks.bin capacity full_disk_filename... \endverbatim
 *
 * */


/*!
 * \page installation_old Installation (Linux/g++ - Stxxl versions earlier than 0.9)
 *
 * \section download Download
 *
 * - Download stxxl_0.77.tgz from
 *   <A href="http://sourceforge.net/project/showfiles.php?group_id=131632&package_id=144407&release_id=541515">SourceForge</A>.
 * - Unpack in some directory executing: \c tar \c zfxv \c stxxl_0.77.tgz ,
 * - Change to \c stxxl directory: \c cd \c stxxl ,
 * - Change file \c compiler.make according to your system configuration
 *   - name of your make program
 *   - name of your compiler
 *   - \c S<small>TXXL</small> root directory ( \c directory_where_you_unpacked_the_tar_ball/stxxl )
 * - Run: \verbatim make lib \endverbatim
 * - Run: \verbatim make tests \endverbatim (optional, if you want to run some test programs)
 *
 * In your makefiles of programs that will use \c S<small>TXXL</small> you should include
 * the file \c compiler.make
 * file (add the line 'include ../compiler.make') because it contains a useful variable (STXXL_VARS)
 * that includes all compiler definitions and library paths that you need to compile an
 * \c S<small>TXXL</small> program.
 *
 * For example: <BR> \verbatim g++  my_example.cpp -o my_example -g $(STXXL_VARS) \endverbatim
 *
 * Before you try to run one of the \c S<small>TXXL</small> examples
 * (or your \c S<small>TXXL</small> program) you must configure the disk
 * space that will be used as external memory for the library. See the next section.
 *
 *
 * \section space Disk space
 *
 * To get best performance with \c S<small>TXXL</small> you should assign separate disks to it.
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your
 * external memory application will increase if you use more than one disk.
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * of about 50-75 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 *
 *
 * \section filesystem Recommended file system
 *
 * Our library take benefit of direct user memory - disk transfers (direct access) which avoids
 * superfluous copies. This method has some disadvantages when accessing files on \c ext2 partitions.
 * Namely one requires one byte of internal memory per each accessed kilobyte of file space. For external
 * memory applications with large inputs this could be not proper. Therefore we recommend to use the
 * \c XFS file system <A href="http://oss.sgi.com/projects/xfs/">link</A> which does not have this overhead but
 * gives the same read and write performance. Note that file creation speed of \c XFS is slow, so that disk
 * files must be precreated.
 *
 *
 * \section configuration Disk configuration file
 *
 * You must define the disk configuration for an
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside
 * in the same directory where you execute the program.
 * You can change the default file name for the configuration
 * file by setting the environment variable \c STXXLCFG .
 *
 * Each line of the configuration file describes a disk.
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file
 *   access methods. Each disk is represented as a file. If you have a disk that is mounted in Unix
 *   to the path /mnt/disk0/, then the correct value for the \c full_disk_filename would be
 *   \c /mnt/disk0/some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different
 *   file access implementations for POSIX systems, choose one of them:
 *   - \c syscall uses \c read and \c write system calls which perform disk transfers directly
 *     on user memory pages without superfluous copying (currently the fastest method)
 *   - \c mmap : performs disks transfers using \c mmap and \c munmap system calls
 *   - \c simdisk : simulates timings of the IBM IC35L080AVVA07 disk, full_disk_filename must point
 *     to a file on a RAM disk partition with sufficient space
 *
 * See also the example configuration file \c 'config_example' included in the tarball.
 *
 *
 * \section excreation Precreating external memory files
 *
 * In order to get the maximum performance one should precreate disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The precreation utility is included in the set of \c S<small>TXXL</small>
 * utilities ( \c utils/createdisks.bin ). Run this utility
 * for each disk you have defined in the disk configuration file:
 * \verbatim utils/createdisks.bin capacity full_disk_filename... \endverbatim
 *
 * */
