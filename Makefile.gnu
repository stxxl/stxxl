# This -*- Makefile -*- is intended for processing with GNU make.

############################################################################
#  Makefile.gnu
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
#  Copyright (C) 2007-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################


TOPDIR	?= $(CURDIR)

main: library

include make.settings

SUBDIRS	= common io mng containers algo stream lib utils contrib

SUBDIRS-lib: $(SUBDIRS:%=lib-in-%)
SUBDIRS-tests: $(SUBDIRS:%=tests-in-%)
SUBDIRS-clean: $(SUBDIRS:%=clean-in-%)

# compute STXXL_CPPFLAGS/STXXL_LDLIBS for stxxl.mk
# don't include optimization, warning and debug flags
stxxl_mk_cppflags	+= $(STXXL_CPPFLAGS_CXX)
stxxl_mk_ldlibs		+= $(STXXL_LDLIBS_CXX)
stxxl_mk_cppflags	+= $$(STXXL_CPPFLAGS_STXXL)
stxxl_mk_ldlibs		+= $$(STXXL_LDLIBS_STXXL)
ifeq ($(strip $(USE_PMODE)),yes)
stxxl_mk_cppflags	+= $$(STXXL_CPPFLAGS_PARALLEL_MODE)
stxxl_mk_ldlibs		+= $$(STXXL_LDLIBS_PARALLEL_MODE)
endif
ifeq ($(strip $(USE_MCSTL)),yes)
stxxl_mk_cppflags	+= $$(STXXL_CPPFLAGS_MCSTL)
stxxl_mk_ldlibs		+= $$(STXXL_LDLIBS_MCSTL)
endif
ifeq ($(strip $(USE_BOOST)),yes)
stxxl_mk_cppflags	+= $$(STXXL_CPPFLAGS_BOOST)
stxxl_mk_ldlibs		+= $$(STXXL_LDLIBS_BOOST)
endif

# dummy target for initial creation of make.settings.local
config:

lib-in-lib:
	@# nothing to compile
lib-in-%:
	$(MAKE) -C $* lib

build-lib: SUBDIRS-lib
	$(RM) $(LIBNAME).stamp
	$(MAKE) -C lib
	touch $(LIBNAME).stamp

ifeq (,$(wildcard lib/lib$(LIBNAME).$(LIBEXT)))
$(LIBNAME).stamp: build-lib
else
$(LIBNAME).stamp:
	$(MAKE) -f Makefile.gnu build-lib
endif

build-lib-utils: $(LIBNAME).stamp
	$(MAKE) -C common tools
	$(MAKE) -C utils tools
	$(MAKE) -C io tools

$(MODENAME).mk:
	$(RM) $(MODENAME).mk.tmp
	echo 'STXXL_CXX			 = $(COMPILER)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_CPPFLAGS			 = $(stxxl_mk_cppflags)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_LDLIBS			 = $(stxxl_mk_ldlibs)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_CPPFLAGS_STXXL		 = $(STXXL_SPECIFIC)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_LDLIBS_STXXL		 = $(STXXL_LDFLAGS) $(STXXL_LDLIBS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_LIBDEPS			 = $(STXXL_LIBDEPS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_CPPFLAGS_PARALLEL_MODE	 = $(PARALLEL_MODE_CPPFLAGS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_LDLIBS_PARALLEL_MODE	 = $(PARALLEL_MODE_LDFLAGS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_CPPFLAGS_MCSTL		 = $(MCSTL_CPPFLAGS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_LDLIBS_MCSTL		 = $(MCSTL_LDFLAGS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_CPPFLAGS_BOOST		 = $(BOOST_COMPILER_OPTIONS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_LDLIBS_BOOST		 = $(BOOST_LINKER_OPTIONS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_WARNFLAGS			 = $(WARNINGS)'	>> $(MODENAME).mk.tmp
	echo 'STXXL_DEBUGFLAGS		 = $(DEBUG)'	>> $(MODENAME).mk.tmp
	cmp -s $(MODENAME).mk.tmp $(MODENAME).mk || mv $(MODENAME).mk.tmp $(MODENAME).mk
	$(RM) $(MODENAME).mk.tmp

.PHONY: $(MODENAME).mk

library: build-lib
library_utils: $(LIBNAME).stamp build-lib-utils $(MODENAME).mk

# skip recompilation of existing library
library-fast:
ifeq (,$(wildcard lib/lib$(LIBNAME).$(LIBEXT)))
library-fast: library_utils
endif

_have_svn_repo	:= $(wildcard .svn)
_have_git_repo	:= $(wildcard .git)

ifneq (,$(_have_svn_repo)$(_have_git_repo))
lib-in-common: common/version_svn.defs

GET_SVNVERSION		?= LC_ALL=POSIX svnversion $(realpath $1)
GET_SVN_INFO		?= LC_ALL=POSIX svn info $(realpath $1)
GET_SVN_INFO_SED	?= sed
GET_SVN_INFO_DATE	?= $(call GET_SVN_INFO, $1) | $(GET_SVN_INFO_SED) -n -e '/Last Changed Date/{' -e 's/.*: //' -e 's/ .*//' -e 's/-//g' -e 'p' -e '}'
GET_SVN_INFO_REV	?= $(call GET_SVN_INFO, $1) | $(GET_SVN_INFO_SED) -n -e '/Last Changed Rev/s/.*: //p'
GET_SVN_INFO_BRANCH	?= $(call GET_SVN_INFO, $1) | $(GET_SVN_INFO_SED) -n -e '/URL/{' -e 's/.*\/svnroot\/stxxl//' -e '/branches/s/\/branches\///p' -e '}'

ifneq (,$(_have_svn_repo))

STXXL_SVN_BRANCH	:= $(shell $(call GET_SVN_INFO_BRANCH, .))

ifeq (,$(strip $(shell svnversion . | tr -d 0-9)))
# clean checkout - use svn info
STXXL_VERSION_DATE	:= $(shell $(call GET_SVN_INFO_DATE, .))
STXXL_VERSION_SVN_REV	:= $(shell $(call GET_SVN_INFO_REV, .))
else
# modified, mixed, ... checkout - use svnversion and today
STXXL_VERSION_DATE	:= $(shell date "+%Y%m%d")
STXXL_VERSION_SVN_REV	:= $(shell $(call GET_SVNVERSION, .))
endif

else # git

STXXL_VERSION_GIT_REV	:= $(shell git describe --all --always --dirty --long)
STXXL_VERSION_DATE	:= $(shell $(if $(findstring dirty,$(STXXL_VERSION_GIT_REV)),date "+%Y%m%d",git log -1 --date=short | awk '/^Date/ { gsub("-",""); print $$2 }'))

endif

# get the svn revision of the MCSTL, if possible
ifneq (,$(strip $(MCSTL_ROOT)))
ifneq (,$(wildcard $(MCSTL_ROOT)/.svn))
ifeq (,$(strip $(shell $(call GET_SVNVERSION, $(MCSTL_ROOT)) | tr -d 0-9)))
# clean checkout - use svn info
MCSTL_VERSION_DATE	:= $(shell $(call GET_SVN_INFO_DATE, $(MCSTL_ROOT)))
MCSTL_VERSION_SVN_REV	:= $(shell $(call GET_SVN_INFO_REV, $(MCSTL_ROOT)))
else
# modified, mixed, ... checkout - use svnversion and today
MCSTL_VERSION_DATE	:= $(shell date "+%Y%m%d")
MCSTL_VERSION_SVN_REV	:= $(shell $(call GET_SVNVERSION, $(MCSTL_ROOT)))
endif
endif
endif

.PHONY: common/version_svn.defs
common/version_svn.defs:
	$(RM) $@.$(LIBNAME).tmp
	echo '#define STXXL_VERSION_STRING_DATE "$(STXXL_VERSION_DATE)"' >> $@.$(LIBNAME).tmp
	$(if $(STXXL_VERSION_SVN_REV),echo '#define STXXL_VERSION_STRING_SVN_REVISION "$(STXXL_VERSION_SVN_REV)"' >> $@.$(LIBNAME).tmp)
	$(if $(STXXL_VERSION_GIT_REV),echo '#define STXXL_VERSION_STRING_GIT_REVISION "$(STXXL_VERSION_GIT_REV)"' >> $@.$(LIBNAME).tmp)
	$(if $(STXXL_SVN_BRANCH), echo '#define STXXL_VERSION_STRING_SVN_BRANCH "$(STXXL_SVN_BRANCH)"' >> $@.$(LIBNAME).tmp)
	$(if $(MCSTL_VERSION_SVN_REV), echo '#define MCSTL_VERSION_STRING_DATE "$(MCSTL_VERSION_DATE)"' >> $@.$(LIBNAME).tmp)
	$(if $(MCSTL_VERSION_SVN_REV), echo '#define MCSTL_VERSION_STRING_SVN_REVISION "$(MCSTL_VERSION_SVN_REV)"' >> $@.$(LIBNAME).tmp)
	cmp -s $@ $@.$(LIBNAME).tmp || mv $@.$(LIBNAME).tmp $@
	$(RM) $@.$(LIBNAME).tmp
endif


tests-in-%: library-fast
	$(MAKE) -C $* tests

tests: SUBDIRS-tests


clean-in-%:
	$(MAKE) -C $* clean

clean: SUBDIRS-clean
	$(RM) common/version_svn.defs
	$(RM) $(LIBNAME).stamp $(MODENAME).stamp $(MODENAME).mk $(MODENAME).mk.tmp


getmodesuffix:
	@echo "$(LIBEXTRA)"


ifneq (,$(_have_svn_repo)$(_have_git_repo))
VERSION		?= $(shell grep 'define *STXXL_VERSION_STRING_MA_MI_PL' common/version.cpp | cut -d'"' -f2)
PHASE		?= snapshot
DATE		?= $(shell date "+%Y%m%d")
REL_VERSION	:= $(VERSION)$(if $(strip $(DATE)),-$(DATE))
release:
	$(RM) -r reltmp stxxl-$(REL_VERSION).tar.gz stxxl-$(REL_VERSION).zip
	mkdir reltmp
ifneq (,$(_have_svn_repo))
	svn export . reltmp/stxxl-$(REL_VERSION)
else
	git archive --prefix=stxxl-$(REL_VERSION)/ HEAD | tar xf - -C reltmp
endif
	$(RM) -r reltmp/stxxl-$(REL_VERSION)/homepage
	find reltmp -name .gitignore -exec rm {} +
	echo '#define STXXL_VERSION_STRING_PHASE "$(PHASE)"' > reltmp/stxxl-$(REL_VERSION)/common/version.defs
	$(if $(strip $(DATE)),echo '#define STXXL_VERSION_STRING_DATE "$(DATE)"' >> reltmp/stxxl-$(REL_VERSION)/common/version.defs)
	cd reltmp && tar cf - stxxl-$(REL_VERSION) | gzip -9 > ../stxxl-$(REL_VERSION).tar.gz && zip -r ../stxxl-$(REL_VERSION).zip stxxl-$(REL_VERSION)/* 
	$(RM) -r reltmp
	@echo
	@echo "Your release has been created in stxxl-$(REL_VERSION).tar.gz and stxxl-$(REL_VERSION).zip"
	@echo "The following files are modified and not committed:"
ifneq (,$(_have_svn_repo))
	@svn status -q
else
	@git status -s
endif
endif


.PHONY: main config library library-fast tests clean release
