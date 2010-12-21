# This -*- Makefile -*- is intended for processing with GNU make.

############################################################################
#  make.settings.gnu
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
#  Copyright (C) 2006-2008 Johannes Singler <singler@ira.uka.de>
#  Copyright (C) 2007-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################


TOPDIR	?= $(error TOPDIR not defined) # DO NOT CHANGE! This is set elsewhere.

# Change this file according to your paths.

# Instead of modifying this file, you could also set your modified variables
# in make.settings.local (needs to be created first).
-include $(TOPDIR)/make.settings.local


USE_BOOST	?= no	# set 'yes' to use Boost libraries or 'no' to not use Boost libraries
USE_MACOSX	?= no	# set 'yes' if you run Mac OS X, 'no' otherwise
USE_FREEBSD	?= no	# set 'yes' if you run FreeBSD, 'no' otherwise
ENABLE_SHARED	?= no   # set 'yes' to build a shared library instead of a static library (EXPERIMENTAL)
USE_PMODE	?= no	# will be overridden from main Makefile
USE_MCSTL	?= no	# will be overridden from main Makefile
USE_ICPC	?= no	# will be overridden from main Makefile

STXXL_ROOT	?= $(TOPDIR)

ifeq ($(strip $(USE_ICPC)),yes)
COMPILER_ICPC	?= icpc
COMPILER	?= $(COMPILER_ICPC)
#ICPC_GCC	?= gcc-x.y    # override the gcc/g++ used to find headers and libraries
WARNINGS	?= -Wall -w1 -openmp-report0 -vec-report0
endif

ifeq ($(strip $(USE_PMODE)),yes)
COMPILER_GCC	?= g++-4.4.x
MODEBASE	?= pmstxxl
endif

ifeq ($(strip $(USE_MCSTL)),yes)
COMPILER_GCC	?= g++-4.2.3
MODEBASE	?= mcstxxl
# the root directory of your MCSTL installation
MCSTL_ROOT	?= $(HOME)/work/mcstl
endif

ifeq ($(strip $(USE_BOOST)),yes)
#BOOST_ROOT	?= /usr/local/boost-1.34.1
LIBEXTRA	?= _boost
endif

COMPILER_GCC	?= g++
COMPILER	?= $(COMPILER_GCC)
LINKER		?= $(COMPILER)
OPT_LEVEL	?= 3
OPT		?= -O$(OPT_LEVEL) # compiler optimization level
WARNINGS	?= -W -Wall -Woverloaded-virtual -Wundef
DEBUG		?= # put here -g option to include the debug information into the binaries

LIBBASE		?= stxxl
LIBEXTRA	?=
MODEBASE	?= stxxl

LIBNAME		?= $(LIBBASE)$(LIBEXTRA)
MODENAME	?= $(MODEBASE)$(LIBEXTRA)

# Hint: for g++-4.4 with c++0x support, enable the following:
#STXXL_SPECIFIC	+= -std=c++0x


#### TROUBLESHOOTING
#
# For automatical checking of order of the output elements in
# the sorters: stxxl::stream::sort, stxxl::stream::merge_runs,
# stxxl::sort, and stxxl::ksort use
#
#STXXL_SPECIFIC	+= -DSTXXL_CHECK_ORDER_IN_SORTS
#
# If your program aborts with message "read/write: wrong parameter"
# or "Invalid argument"
# this could be that your kernel does not support direct I/O
# then try to set it off recompiling the libs and your code with option
#
#STXXL_SPECIFIC	+= -DSTXXL_DIRECT_IO_OFF
#
# But for the best performance it is strongly recommended
# to reconfigure the kernel for the support of the direct I/O.


#### You usually shouldn't need to change the sections below #####


#### MACOSX CONFIGURATION ########################################

ifeq ($(strip $(USE_MACOSX)),yes)

PTHREAD_FLAG	?=

GET_FILE_ID	?= stat -L -f '%d:%i' $1

endif

##################################################################


#### FREEBSD CONFIGURATION #######################################

ifeq ($(strip $(USE_FREEBSD)),yes)

PTHREAD_FLAG	?= -pthread

GET_FILE_ID	?= stat -L -f '%d:%i' $1

endif

##################################################################


#### LINUX (DEFAULT) CONFIGURATION ###############################

PTHREAD_FLAG	?= -pthread

# get a unique identifier for a file or directory,
# e.g. device number + inode number
GET_FILE_ID	?= stat -L -c '%d:%i' $1

##################################################################


#### SHARED LIBRARY ###############################################

ifeq ($(strip $(ENABLE_SHARED)),yes)

LIBEXT	?= so
LIBGEN	?= false  # see lib/GNUmakefile
STXXL_LIBRARY_SPECIFIC	+= -fPIC

endif

###################################################################


#### STXXL CONFIGURATION #########################################

# create make.settings.local in the root directory
ifneq (,$(strip $(wildcard $(CURDIR)/include/stxxl.h)))
ifeq (,$(strip $(wildcard $(CURDIR)/make.settings.local)))
ifeq (,$(STXXL_AUTOCONFIG))
$(warning *** WARNING: you did not have a make.settings.local file -- creating ...)
endif
cmt	= \#
$(shell echo '$(cmt)STXXL_ROOT	 = $(CURDIR:$(HOME)%=$$(HOME)%)' >> $(CURDIR)/make.settings.local)
MCSTL_ROOT	?= $(HOME)/work/mcstl
$(shell echo '$(cmt)MCSTL_ROOT	 = $(MCSTL_ROOT:$(HOME)%=$$(HOME)%)' >> $(CURDIR)/make.settings.local)
$(shell echo '$(cmt)COMPILER_GCC	 = g++-4.2' >> $(CURDIR)/make.settings.local)
$(shell echo '$(cmt)COMPILER_GCC	 = g++-4.4 -std=c++0x' >> $(CURDIR)/make.settings.local)
$(shell echo '$(cmt)COMPILER_ICPC	 = icpc' >> $(CURDIR)/make.settings.local)
$(shell echo '$(cmt)USE_BOOST	 = no' >> $(CURDIR)/make.settings.local)
$(shell echo '$(cmt)BOOST_ROOT	 = ' >> $(CURDIR)/make.settings.local)
ifeq (Darwin,$(strip $(shell uname)))
$(shell echo 'USE_MACOSX	 = yes' >> $(CURDIR)/make.settings.local)
else
$(shell echo '$(cmt)USE_MACOSX	 = no' >> $(CURDIR)/make.settings.local)
endif
ifeq (FreeBSD,$(strip $(shell uname)))
$(shell echo 'USE_FREEBSD	 = yes' >> $(CURDIR)/make.settings.local)
else
$(shell echo '$(cmt)USE_FREEBSD	 = no' >> $(CURDIR)/make.settings.local)
endif
include make.settings.local
endif
endif

# check, whether stxxl is configured correctly
ifeq (,$(strip $(wildcard $(STXXL_ROOT)/include/stxxl.h)))
$(warning *** WARNING: STXXL has not been configured correctly)
$(warning *** WARNING: Please check make.settings.local)
$(error ERROR: could not find a STXXL installation in STXXL_ROOT=$(STXXL_ROOT))
endif

# in the top dir, check whether STXXL_ROOT points to ourselves
ifneq (,$(wildcard make.settings))
stat1	 = $(shell $(call GET_FILE_ID, ./))
stat2	 = $(shell $(call GET_FILE_ID, $(STXXL_ROOT)/))

ifneq ($(stat1),$(stat2))
$(error ERROR: STXXL_ROOT=$(STXXL_ROOT) points to a different STXXL installation)
endif
endif

##################################################################


#### STXXL OPTIONS ###############################################

STXXL_SPECIFIC	+= \
	$(PTHREAD_FLAG) \
	$(CPPFLAGS_ARCH) \
	-I$(strip $(STXXL_ROOT))/include \
	-include stxxl/bits/defines.h \
	-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

STXXL_LIBRARY_SPECIFIC	+= -D_IN_LIBSTXXL

STXXL_LDFLAGS	+= $(PTHREAD_FLAG)
STXXL_LDLIBS	+= -L$(strip $(STXXL_ROOT))/lib -l$(LIBNAME)

STXXL_LIBDEPS	+= $(strip $(STXXL_ROOT))/lib/lib$(LIBNAME).$(LIBEXT)

UNAME_M		:= $(shell uname -m)
CPPFLAGS_ARCH	+= $(CPPFLAGS_$(UNAME_M))
CPPFLAGS_i686	?= -march=i686

##################################################################


#### ICPC OPTIONS ################################################

ifeq ($(strip $(USE_ICPC)),yes)

OPENMPFLAG	?= -openmp

ICPC_CPPFLAGS	+= $(if $(ICPC_GCC),-gcc-name=$(strip $(ICPC_GCC)))
ICPC_LDFLAGS	+= $(if $(ICPC_GCC),-gcc-name=$(strip $(ICPC_GCC)))

STXXL_SPECIFIC	+= -include bits/intel_compatibility.h

endif

##################################################################


#### PARALLEL_MODE OPTIONS ###############################################

ifeq ($(strip $(USE_PMODE)),yes)

OPENMPFLAG			?= -fopenmp

PARALLEL_MODE_CPPFLAGS		+= $(OPENMPFLAG) -D_GLIBCXX_PARALLEL
PARALLEL_MODE_LDFLAGS		+= $(OPENMPFLAG)

endif

##################################################################


#### MCSTL OPTIONS ###############################################

ifeq ($(strip $(USE_MCSTL)),yes)

OPENMPFLAG	?= -fopenmp

ifeq (,$(strip $(wildcard $(strip $(MCSTL_ROOT))/c++/mcstl.h)))
$(error ERROR: could not find a MCSTL installation in MCSTL_ROOT=$(MCSTL_ROOT))
endif

MCSTL_CPPFLAGS		+= $(OPENMPFLAG) -D__MCSTL__ -I$(MCSTL_ROOT)/c++
MCSTL_LDFLAGS		+= $(OPENMPFLAG)

endif

##################################################################


#### BOOST OPTIONS ###############################################

BOOST_INCLUDE			?= $(if $(strip $(BOOST_ROOT)),$(strip $(BOOST_ROOT))/include)
BOOST_COMPILER_OPTIONS		+= $(foreach inc,$(BOOST_INCLUDE),-I$(inc))
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_CONFIG
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_FILESYSTEM
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_RANDOM
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_THREADS
#BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_TIMESTAMP   # probably less efficient than gettimeofday()

BOOST_LIB_PATH			?= $(if $(strip $(BOOST_ROOT)),$(strip $(BOOST_ROOT))/lib)
BOOST_LIB_COMPILER_SUFFIX	?=
BOOST_LIB_MT_SUFFIX		?= -mt
BOOST_LINKER_OPTIONS		 = \
	$(foreach lib,$(BOOST_LIB_PATH),-L$(lib)) \
	-lboost_thread$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_date_time$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_iostreams$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_system$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_filesystem$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX)

##################################################################


#### CPPUNIT OPTIONS #############################################

CPPUNIT_COMPILER_OPTIONS	+=

CPPUNIT_LINKER_OPTIONS		+= -lcppunit -ldl

##################################################################


#### DEPENDENCIES ################################################

HEADER_FILES_BITS	+= namespace.h noncopyable.h version.h
HEADER_FILES_BITS	+= compat_hash_map.h
HEADER_FILES_BITS	+= compat_unique_ptr.h parallel.h singleton.h defines.h
HEADER_FILES_BITS	+= verbose.h unused.h compat_type_traits.h
HEADER_FILES_BITS	+= msvc_compatibility.h

HEADER_FILES_COMMON	+= aligned_alloc.h new_alloc.h
HEADER_FILES_COMMON	+= mutex.h rand.h semaphore.h state.h
HEADER_FILES_COMMON	+= timer.h utils.h error_handling.h simple_vector.h
HEADER_FILES_COMMON	+= switch.h tmeta.h log.h exceptions.h tuple.h
HEADER_FILES_COMMON	+= types.h settings.h seed.h is_sorted.h exithandler.h

HEADER_FILES_IO		+= io.h iostats.h completion_handler.h
HEADER_FILES_IO		+= request.h request_waiters_impl_basic.h
HEADER_FILES_IO		+= request_state_impl_basic.h request_impl_basic.h
HEADER_FILES_IO		+= disk_queues.h
HEADER_FILES_IO		+= request_queue.h request_queue_impl_worker.h
HEADER_FILES_IO		+= request_queue_impl_qwqr.h
HEADER_FILES_IO		+= request_queue_impl_1q.h
HEADER_FILES_IO		+= file.h disk_queued_file.h
HEADER_FILES_IO		+= ufs_file_base.h syscall_file.h mmap_file.h simdisk_file.h
HEADER_FILES_IO		+= wfs_file_base.h wincall_file.h
HEADER_FILES_IO		+= boostfd_file.h mem_file.h fileperblock_file.h
HEADER_FILES_IO		+= wbtl_file.h
HEADER_FILES_IO		+= create_file.h

HEADER_FILES_MNG	+= adaptor.h block_prefetcher.h
HEADER_FILES_MNG	+= buf_istream.h buf_ostream.h buf_writer.h mng.h
HEADER_FILES_MNG	+= bid.h typed_block.h diskallocator.h config.h
HEADER_FILES_MNG	+= write_pool.h prefetch_pool.h read_write_pool.h
HEADER_FILES_MNG	+= block_alloc.h block_alloc_interleaved.h

HEADER_FILES_CONTAINERS	+= pager.h stack.h vector.h priority_queue.h
#EADER_FILES_CONTAINERS	+= pq_helpers.h pq_mergers.h pq_ext_merger.h
#EADER_FILES_CONTAINERS	+= pq_losertree.h
HEADER_FILES_CONTAINERS	+= queue.h map.h deque.h

HEADER_FILES_CONTAINERS_BTREE	+= btree.h iterator_map.h leaf.h node_cache.h
HEADER_FILES_CONTAINERS_BTREE	+= root_node.h node.h btree_pager.h iterator.h

HEADER_FILES_ALGO	+= adaptor.h inmemsort.h intksort.h run_cursor.h sort.h
HEADER_FILES_ALGO	+= async_schedule.h ksort.h sort_base.h sort_helper.h
HEADER_FILES_ALGO	+= losertree.h scan.h stable_ksort.h random_shuffle.h

HEADER_FILES_STREAM	+= stream.h sort_stream.h
HEADER_FILES_STREAM	+= choose.h unique.h
HEADER_FILES_STREAM	+= sorted_runs.h

HEADER_FILES_UTILS	+= malloc.h

###################################################################


#### MISC #########################################################

# extension of object files
OBJEXT		?= $(MODENAME).o
# extension of object files for the library
LIBOBJEXT	?= lib$(LIBNAME).o
IIEXT		?= $(MODENAME).ii
# static library file extension
LIBEXT		?= a
# executable file extension
EXEEXT		?= $(MODENAME).bin
# static library generation
LIBGEN		?= ar cr
# output file option for the compiler and linker
OUT		?= -o

o	?= $(strip $(OBJEXT))
lo	?= $(strip $(LIBOBJEXT))
ii	?= $(strip $(IIEXT))
bin	?= $(strip $(EXEEXT))

###################################################################


#### COMPILE/LINK RULES ###########################################

define COMPILE_STXXL
	@$(RM) $@ $(@:.o=).d
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -MD -MF $(@:.o=).dT -c $(OUTPUT_OPTION) $< && mv $(@:.o=).dT $(@:.o=).d
endef

DEPS_MAKEFILES		:= $(wildcard $(TOPDIR)/Makefile.subdir.gnu $(TOPDIR)/make.settings $(TOPDIR)/make.settings.local GNUmakefile Makefile Makefile.common Makefile.local)
EXTRA_DEPS_COMPILE	?= $(DEPS_MAKEFILES)
%.$o: %.cpp $(EXTRA_DEPS_COMPILE)
	$(COMPILE_STXXL)

%.$(lo): PARALLEL_MODE_CPPFLAGS=
%.$(lo): MCSTL_CPPFLAGS=
%.$(lo): STXXL_COMPILER_OPTIONS += $(STXXL_LIBRARY_SPECIFIC)
%.$(lo): o=$(lo)
%.$(lo): %.cpp $(EXTRA_DEPS_COMPILE)
	$(COMPILE_STXXL)

%.$(ii): %.cpp $(EXTRA_DEPS_COMPILE)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -E $(OUTPUT_OPTION) $<

# $1=infix $2=additional CPPFLAGS
define COMPILE_VARIANT
%.$1.$$o: CPPFLAGS += $2
%.$1.$$o: %.cpp $$(EXTRA_DEPS_COMPILE)
	$$(COMPILE_STXXL)
endef

LINK_STXXL	 = $(LINKER) $1 $(STXXL_LINKER_OPTIONS) -o $@

%.$(bin): %.$o $(STXXL_LIBDEPS)
	$(call LINK_STXXL, $<)


# last resort rules to ignore header files missing due to renames etc.
$(STXXL_ROOT)/include/%::
	@echo "MISSING HEADER: '$@' (ignored)"

%.h::
	@echo "MISSING HEADER: '$@' (ignored)"

/%::
	@echo "MISSING FILE:   '$@' (ignored)"

###################################################################


STXXL_SPECIFIC		+= $(STXXL_EXTRA)
WARNINGS		+= $(WARNINGS_EXTRA)

ifeq ($(strip $(USE_ICPC)),yes)
STXXL_CPPFLAGS_CXX	+= $(ICPC_CPPFLAGS)
STXXL_LDLIBS_CXX	+= $(ICPC_LDFLAGS)
endif

STXXL_COMPILER_OPTIONS	+= $(STXXL_CPPFLAGS_CXX)
STXXL_COMPILER_OPTIONS	+= $(STXXL_SPECIFIC)
STXXL_COMPILER_OPTIONS	+= $(OPT) $(DEBUG) $(WARNINGS)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDLIBS_CXX)
STXXL_LINKER_OPTIONS	+= $(DEBUG)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDFLAGS)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDLIBS)

ifeq ($(strip $(USE_PMODE)),yes)
STXXL_COMPILER_OPTIONS	+= $(PARALLEL_MODE_CPPFLAGS)
STXXL_LINKER_OPTIONS	+= $(PARALLEL_MODE_LDFLAGS)
endif

ifeq ($(strip $(USE_MCSTL)),yes)
STXXL_COMPILER_OPTIONS	+= $(MCSTL_CPPFLAGS)
STXXL_LINKER_OPTIONS	+= $(MCSTL_LDFLAGS)
endif

ifeq ($(strip $(USE_BOOST)),yes)
STXXL_COMPILER_OPTIONS	+= $(BOOST_COMPILER_OPTIONS)
STXXL_LINKER_OPTIONS	+= $(BOOST_LINKER_OPTIONS)
endif

STXXL_COMPILER_OPTIONS	+= $(CPPFLAGS)

# vim: syn=make
