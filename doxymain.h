/*! \mainpage Documentation for <small>STXXL</small> library
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
 * - SunOS
 * - Windows XP/2000
 *  
 * 
 * Questions concerning use and development of the \c S<small>TXXL</small> library post
 * to <b><a href="https://sourceforge.net/forum/?group_id=131632">FORUMS</a></b> or mail to <A 
 * href="http://i10www.ira.uka.de/dementiev/" >Roman
 *  Dementiev</A>.
 *
 * - \link installation_linux_g++ Installation (Linux/g++) \endlink
 * - \link installation_sunos_g++ Installation (SunOS/g++) \endlink
 * - \link installation_msvc Installation (Windows/MS Visual C++ 7.1) \endlink
 * - \link installation_old Installation of the older Stxxl versions (earlier than 0.9) (Linux/g++) \endlink
 * 
 *
 */

/*!
 * \page installation Installation
 * - \link installation_linux_g++ Installation (Linux/g++) \endlink
 * - \link installation_sunos_g++ Installation (SunOS/g++) \endlink
 * - \link installation_msvc Installation (Windows/MS Visual C++ 7.1) \endlink
 * - \link installation Installation of the older Stxxl versions (earlier than 0.9) (Linux/g++) \endlink
 */

/*!
 * \page installation_linux_g++ Installation (Linux/g++ - Stxxl from version 0.9)
 *
 * \section download Download
 *
 * - Download gzipped tar ball from <A href="http://i10www.ira.uka.de/dementiev/files/stxxl.tgz">here</A>. 
 * - Unpack in some directory executing: \c tar \c zfxv \c stxxl.tgz ,
 * - Change to \c stxxl directory: \c cd \c stxxl ,
 * - Change \c make.settings.g++ file according to your system configuration
 *   - \c S<small>TXXL</small> root directory \c STXXL_ROOT variable( \c directory_where_you_unpacked_the_tar_ball/stxxl )
 *   - change USE_BOOST variable to yes if you want Stxxl to use <A href="http://www.boost.org">Boost</A> libraries (you should have Boost libraries already installed)
 *   - change BOOST_INCLUDE variable according to the Boost include path (if you set USE_BOOST to 'yes')
 *   - set OPT variable to -O3 or other g++ optimization level you like (optionally)
 *   - set DEBUG variable to -g or other g++ debugging option if you want to produce debug version of the Stxxl library or Stxxl examples
 * - Run: \verbatim make library_g++ \endverbatim
 * - Run: \verbatim make tests_g++ \endverbatim (optional, if you want to compile and run some test programs)
 * 
 * Programs using Stxxl can be compiled using g++ command line options from \c compiler.options 
 * file. The linking options you can find in \c linker.options file. Alternatively you can
 * include \c make.settings file in your make files and use \c STXXL_COMPILER_OPTIONS and 
 * \c STXXL_LINKER_OPTIONS variables, defined therein.
 *
 * For example: <BR> 
 * \verbatim g++ -c my_example.cpp $(STXXL_COMPILER_OPTIONS) \endverbatim <BR>
 * \verbatim g++ my_example.o -o my_example.bin $(STXXL_LINKER_OPTIONS) \endverbatim
 * 
 * Before you try to run one of the \c S<small>TXXL</small> examples 
 * (or your own \c S<small>TXXL</small> program) you must configure the disk 
 * space that will be used as external memory for the library. For instructions how to do that, 
 * see the next section.
 *
 *
 * \section space Disk space
 *
 * To get best performance with \c S<small>TXXL</small>  you should assign separate disks to it. 
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your 
 * external memory application will increase if you use more than one disk. 
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * about of 50-75 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 *
 * \section filesystem Recommended file system
 * 
 * Our library take benefit of direct user memory - disk transfers (direct access) which avoids 
 * superfluous copies.  
 * We recommend to use the 
 * \c XFS file system <A href="http://oss.sgi.com/projects/xfs/">link</A> that
 * gives good read and write performance for large files. 
 * Note that file creation speed of \c XFS is slow, so that disk
 * files should be precreated.
 * 
 * \section configuration Disk configuration file
 * 
 * You must define the disk configuration for an 
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside 
 * in the same directory where you execute the program. 
 * You can change the default file name for the configuration 
 * file by setting the enviroment variable \c STXXLCFG . 
 *
 *
 * Each line of the configuration file describes a disk. 
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file 
 * access methods. Each disk is respresented as a file. If you have a disk that is mapped in unix
 * to the path /mnt/disk0/, then the correct value for the \c full_disk_filename would be
 * \c /mnt/disk0/some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different file access implementations for POSIX systems, choose one of them:
 *   - \c syscall uses \c read and \c write system calls which perform disk transfers directly 
 *   on user memory pages without superfluous copy (currently the fastest method)
 *   - \c mmap : performs disks transfers using \c mmap and \c munmap system calls
 *   - \c simdisk : simulates timings of the IBM IC35L080AVVA07 disk, full_disk_filename must point 
 *     to a file on a RAM disk partition with sufficient space
 *
 *
 * See also example configuration file \c 'stxxl/config_example' included into the tarball.
 * 
 * \section excreation Formatting external memory files
 *
 * In order to get the maximum performance one should format disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The format utility is included into the set of \c S<small>TXXL</small> utilities ( \c utils/createdisks.bin ). Run this utility
 * for each disk you have defined in the disk configuration file.
 *
 * */


/*!
 * \page installation_msvc Installation (Windows/MS Visual C++ 7.1 - Stxxl from version 0.9)
 *
 * \section download Download
 *
 * - Install the <a href="http://www.boost.org">Boost</a> libraries (required).
 * - Download the \c Stxxl zip file from <A href="http://i10www.ira.uka.de/dementiev/files/stxxl.zip">here</A>. 
 * - Unpack the zip file in some directory (e.g. \c 'c:\\'),
 * - Change to \c stxxl directory: \c cd \c stxxl ,
 * - Change \c make.settings.msvc file according to your system configuration
 *   - \c S<small>TXXL</small> root directory \c STXXL_ROOT variable( \c directory_where_you_unpacked_the_tar_ball\\stxxl , e.g. \c 'c:\\stxxl')
 *   - change BOOST_ROOT variable according to the Boost root path
 *   - set OPT variable to /O2 or other VC++ optimization level you like (optionally)
 *   - set DEBUG variable to /MDd for debug version of the \c Stxxl library or to /MD for the version without debugging information in object files
 * - Open the \c stxxl.vcproj file (VS Solution Object) in Visual Studio .NET.  The file is located in the \c STXXL_ROOT directory
 * - Press F7 to build the library. The library file (libstxxl.lib) should appear in \c STXXL_ROOT\\lib directory
 * - In the configuration manager ('Build' drop-down menu) choose 'Library and tests' as active solution configuration. Press OK.
 * - Press F7 to build \c stxxl test programs.
 * 
 * Programs using Stxxl can be compiled using options from \c compiler.options 
 * file (in the \c STXXL_ROOT directory). The linking options for the VC++ 
 * linker you can find in \c linker.options file. In order to accomplish this
 * do the following:
 * - Open project property pages (menu Progect->Properties)
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
 * Before you try to run one of the \c S<small>TXXL</small> examples 
 * (or your own \c S<small>TXXL</small> program) you must configure the disk 
 * space that will be used as external memory for the library. For instructions how to do that, 
 * see the next section.
 *
 * <BR>
 * The \c STXXL_ROOT\\test\\WinGUI directory contains an example MFC GUI project
 * that uses \c Stxxl. In order to compile it open the WinGUI.vcproj file in
 * Visual Studio .NET. Change if needed the Compiler and Linker Options of the project
 * (see above).
 *
 * \section space Disk space
 *
 * To get best performance with \c S<small>TXXL</small>  you should assign separate disks to it. 
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your 
 * external memory application will increase if you use more than one disk. 
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * about of 50-75 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 * 
 * \section configuration Disk configuration file
 * 
 * You must define the disk configuration for an 
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside 
 * in the same directory where you execute the program. 
 * You can change the default file name for the configuration 
 * file by setting the enviroment variable \c STXXLCFG . 
 *
 *
 * Each line of the configuration file describes a disk. 
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file 
 * access methods. Each disk is respresented as a file. If you have a disk called \c e:
 * then the correct value for the \c full_disk_filename would be
 * \c e:\\some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different file access implementations for WINDOWS, choose one of them:
 *   - \c syscall: uses \c read and \c write POSIX system calls (slow)
 *   - \c wincall: performs disks transfers using \c ReadFile and \c WriteFile WinAPI calls
 *   This method supports direct I/O that avoids superfluous copying of data pages 
 *   in the Windows kernel. This is the best (and default) method in Stxxl for Windows.
 *
 * See also example configuration file \c 'STXXL_ROOT\\config_example_win' included into the package.
 * 
 * \section excreation Formatting external memory files
 *
 * In order to get the maximum performance one should precreate disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The precreation utility is included into the set of \c S<small>TXXL</small> utilities ( \c utils\\createdisks.exe ). Run this utility
 * for each disk you have defined in the disk configuration file.
 *
 * 
 * */



/*!
 * \page installation_sunos_g++ Installation (SunOS/g++ - Stxxl from version 0.9)
 *
 * \section download Download
 *
 * - Download gzipped tar ball from <A href="http://i10www.ira.uka.de/dementiev/files/stxxl.tgz">here</A>. 
 * - Unpack in some directory executing: \c tar \c zfxv \c stxxl.tgz ,
 * - Change to \c stxxl directory: \c cd \c stxxl ,
 * - Change \c make.settings.g++ file according to your system configuration
 *   - \c S<small>TXXL</small> root directory \c STXXL_ROOT variable( \c directory_where_you_unpacked_the_tar_ball/stxxl )
 *   - change USE_BOOST variable to 'yes' if you want Stxxl to use <A href="http://www.boost.org">Boost</A> libraries (you should have Boost libraries already installed)
 *   - change USE_LINUX variable to 'no'
 *   - change BOOST_INCLUDE variable according to the Boost include path (if you set USE_BOOST to 'yes')
 *   - set OPT variable to -O3 or other g++ optimization level you like (optionally)
 *   - set DEBUG variable to -g or other g++ debugging option if you want to produce debug version of the Stxxl library or Stxxl examples
 * - Run: \verbatim make library_g++ \endverbatim
 * - Run: \verbatim make tests_g++ \endverbatim (optional, if you want to compile and run some test programs)
 * 
 * Programs using Stxxl can be compiled using g++ command line options from \c compiler.options 
 * file. The linking options you can find in \c linker.options file. Alternatively you can
 * include \c make.settings file in your make files and use \c STXXL_COMPILER_OPTIONS and 
 * \c STXXL_LINKER_OPTIONS variables, defined therein.
 *
 * For example: <BR> 
 * \verbatim g++ -c my_example.cpp $(STXXL_COMPILER_OPTIONS) \endverbatim <BR>
 * \verbatim g++ my_example.o -o my_example.bin $(STXXL_LINKER_OPTIONS) \endverbatim
 * 
 * Before you try to run one of the \c S<small>TXXL</small> examples 
 * (or your own \c S<small>TXXL</small> program) you must configure the disk 
 * space that will be used as external memory for the library. For instructions how to do that, 
 * see the next section.
 *
 *
 * \section space Disk space
 *
 * To get best performance with \c S<small>TXXL</small>  you should assign separate disks to it. 
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your 
 * external memory application will increase if you use more than one disk. 
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * about of 50-75 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 *
 * \section filesystem Recommended file system
 * 
 * Our library take benefit of direct user memory - disk transfers (direct access) which avoids 
 * superfluous copies.  
 * We recommend to use the 
 * \c XFS file system <A href="http://oss.sgi.com/projects/xfs/">link</A> that
 * gives good read and write performance for large files. 
 * Note that file creation speed of \c XFS is slow, so that disk
 * files should be precreated.
 * 
 * \section configuration Disk configuration file
 * 
 * You must define the disk configuration for an 
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside 
 * in the same directory where you execute the program. 
 * You can change the default file name for the configuration 
 * file by setting the enviroment variable \c STXXLCFG . 
 *
 *
 * Each line of the configuration file describes a disk. 
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file 
 * access methods. Each disk is respresented as a file. If you have a disk that is mapped in unix
 * to the path /mnt/disk0/, then the correct value for the \c full_disk_filename would be
 * \c /mnt/disk0/some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different file access implementations for POSIX systems, choose one of them:
 *   - \c syscall uses \c read and \c write system calls which perform disk transfers directly 
 *   on user memory pages without superfluous copy (currently the fastest method)
 *   - \c mmap : performs disks transfers using \c mmap and \c munmap system calls
 *   - \c simdisk : simulates timings of the IBM IC35L080AVVA07 disk, full_disk_filename must point 
 *     to a file on a RAM disk partition with sufficient space
 *
 *
 * See also example configuration file \c 'stxxl/config_example' included into the tarball.
 * 
 * \section excreation Formatting external memory files
 *
 * In order to get the maximum performance one should format disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The format utility is included into the set of \c S<small>TXXL</small> utilities ( \c utils/createdisks.bin ). Run this utility
 * for each disk you have defined in the disk configuration file.
 *
 * 
 * */


/*!
 * \page installation_old Installation (Linux/g++ - Stxxl versions earlier than 0.9)
 *
 * \section download Download
 *
 * - Download gzipped tar ball from <A href="http://i10www.ira.uka.de/dementiev/files/stxxl_0.77.tgz">here</A>. 
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
 * To get best performance with \c S<small>TXXL</small>  you should assign separate disks to it. 
 * These disks should be used by the library only.
 * Since \c S<small>TXXL</small> is developed to exploit disk parallelism, the performance of your 
 * external memory application will increase if you use more than one disk. 
 * But from how many disks your application can benefit depends on how "I/O bound" it is.
 * With modern disk bandwidths
 * about of 40-60 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
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
 * \section configuration Disk configuration file
 * 
 * You must define the disk configuration for an 
 * \c S<small>TXXL</small> program in a file named \c '.stxxl' that must reside 
 * in the same directory where you execute the program. 
 * You can change the default file name for the configuration 
 * file by setting the enviroment variable \c STXXLCFG . 
 *
 *
 * Each line of the configuration file describes a disk. 
 * A disk description uses the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks S<small>TXXL</small> uses file 
 * access methods. Each disk is respresented as a file. If you have a disk that is mapped in unix
 * to the path /mnt/disk0/, then the correct value for the \c full_disk_filename would be
 * \c /mnt/disk0/some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c S<small>TXXL</small> has a number of different file access implementations, choose one of them:
 *   - \c syscall uses \c read and \c write system calls which perform disk transfers directly 
 *   on user memory pages without superfluous copy (currently the fastest method)
 *   - \c mmap : performs disks transfers using \c mmap and \c munmap system calls
 *   - \c simdisk : simulates timings of the IBM IC35L080AVVA07 disk, full_disk_filename must point 
 *     to a file on a RAM disk partition with sufficient space
 *
 *
 * See also example configuration file \c 'stxxl/config_example' included into the tarball.
 * 
 * \section excreation Formatting external memory files
 *
 * In order to get the maximum performance one should format disk files described in the configuration file,
 * before running \c S<small>TXXL</small> applications.
 *
 * The format utility is included into a set of \c S<small>TXXL</small> utilities ( \c utils/createdisks ). Run this utility
 * for each disk described in your disk configuration file.
 *
 * 
 * */
