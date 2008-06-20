# This -*- Makefile -*- is intended for processing with GNU make.

TOPDIR	?= .

main: library

include make.settings

SUBDIRS	= common io mng containers algo stream lib utils

SUBDIRS-lib: $(SUBDIRS:%=lib-in-%)
SUBDIRS-tests: $(SUBDIRS:%=tests-in-%)
SUBDIRS-clean: $(SUBDIRS:%=clean-in-%)

# compute STXXL_CPPFLAGS/STXXL_LDLIBS for stxxl.mk
# don't include optimization, warning and debug flags
stxxl_mk_cppflags	+= $(STXXL_CPPFLAGS_CXX)
stxxl_mk_ldlibs		+= $(STXXL_LDLIBS_CXX)
stxxl_mk_cppflags	+= $$(STXXL_CPPFLAGS_STXXL)
stxxl_mk_ldlibs		+= $$(STXXL_LDLIBS_STXXL)
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
	$(MAKE) -C lib
	$(MAKE) -C utils create

$(LIBNAME).stamp: build-lib
	$(RM) $@ $(LIBNAME).mk.tmp
	echo 'STXXL_CXX	 = $(COMPILER)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_CPPFLAGS	 = $(stxxl_mk_cppflags)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_LDLIBS	 = $(stxxl_mk_ldlibs)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_CPPFLAGS_STXXL	 = $(STXXL_SPECIFIC)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_LDLIBS_STXXL	 = $(STXXL_LDFLAGS) $(STXXL_LDLIBS)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_LIBDEPS		 = $(STXXL_LIBDEPS)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_CPPFLAGS_MCSTL	 = $(MCSTL_CPPFLAGS)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_LDLIBS_MCSTL	 = $(MCSTL_LDFLAGS)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_CPPFLAGS_BOOST	 = $(BOOST_COMPILER_OPTIONS)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_LDLIBS_BOOST	 = $(BOOST_LINKER_OPTIONS)'	>> $(LIBNAME).mk.tmp
	echo 'STXXL_WARNFLAGS		 = $(WARNINGS)'	>> $(LIBNAME).mk.tmp
	cmp -s $(LIBNAME).mk.tmp $(LIBNAME).mk || mv $(LIBNAME).mk.tmp $(LIBNAME).mk
	$(RM) $(LIBNAME).mk.tmp
	touch $@

library: $(LIBNAME).stamp

# skip recompilation of existing library
library-fast:
ifeq (,$(wildcard lib/lib$(LIBNAME).$(LIBEXT)))
library-fast: library
endif

ifneq (,$(wildcard .svn))
lib-in-common: common/version_svn.defs

GET_SVN_INFO		?= LC_ALL=POSIX svn info $1
GET_SVN_INFO_SED	?= sed
GET_SVN_INFO_DATE	?= $(call GET_SVN_INFO, $1) | $(GET_SVN_INFO_SED) -n -e '/Last Changed Date/{' -e 's/.*: //' -e 's/ .*//' -e 's/-//g' -e 'p' -e '}'
GET_SVN_INFO_REV	?= $(call GET_SVN_INFO, $1) | $(GET_SVN_INFO_SED) -n -e '/Last Changed Rev/s/.*: //p'
GET_SVN_INFO_BRANCH	?= $(call GET_SVN_INFO, $1) | $(GET_SVN_INFO_SED) -n -e '/URL/{' -e 's/.*\/svnroot\/stxxl//' -e '/branches/s/\/branches\///p' -e '}'

STXXL_SVN_BRANCH	:= $(shell $(call GET_SVN_INFO_BRANCH, .))

ifeq (,$(strip $(shell svnversion . | tr -d 0-9)))
# clean checkout - use svn info
STXXL_VERSION_DATE	:= $(shell $(call GET_SVN_INFO_DATE, .))
STXXL_VERSION_SVN_REV	:= $(shell $(call GET_SVN_INFO_REV, .))
else
# modified, mixed, ... checkout - use svnversion and today
STXXL_VERSION_DATE	:= $(shell date "+%Y%m%d")
STXXL_VERSION_SVN_REV	:= $(shell svnversion .)
endif

# get the svn revision of the MCSTL, if possible
ifneq (,$(strip $(MCSTL_ROOT)))
ifneq (,$(wildcard $(MCSTL_ROOT)/.svn))
ifeq (,$(strip $(shell svnversion $(MCSTL_ROOT) | tr -d 0-9)))
# clean checkout - use svn info
MCSTL_VERSION_DATE	:= $(shell $(call GET_SVN_INFO_DATE, $(MCSTL_ROOT)))
MCSTL_VERSION_SVN_REV	:= $(shell $(call GET_SVN_INFO_REV, $(MCSTL_ROOT)))
else
# modified, mixed, ... checkout - use svnversion and today
MCSTL_VERSION_DATE	:= $(shell date "+%Y%m%d")
MCSTL_VERSION_SVN_REV	:= $(shell svnversion $(MCSTL_ROOT))
endif
endif
endif

.PHONY: common/version_svn.defs
common/version_svn.defs:
	$(RM) $@.$(LIBNAME).tmp
	echo '#define STXXL_VERSION_STRING_DATE "$(STXXL_VERSION_DATE)"' >> $@.$(LIBNAME).tmp
	echo '#define STXXL_VERSION_STRING_SVN_REVISION "$(STXXL_VERSION_SVN_REV)"' >> $@.$(LIBNAME).tmp
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
	$(RM) $(LIBNAME).stamp $(LIBNAME).mk $(LIBNAME).mk.tmp


ifneq (,$(wildcard .svn))
VERSION		?= $(shell grep 'define *STXXL_VERSION_STRING_MA_MI_PL' common/version.cpp | cut -d'"' -f2)
PHASE		?= snapshot
DATE		?= $(shell date "+%Y%m%d")
REL_VERSION	:= $(VERSION)$(if $(strip $(DATE)),-$(DATE))
release:
	$(RM) -r reltmp stxxl-$(REL_VERSION).tar.gz stxxl-$(REL_VERSION).zip
	mkdir reltmp
	svn export . reltmp/stxxl-$(REL_VERSION)
	$(RM) -r reltmp/stxxl-$(REL_VERSION)/homepage
	echo '#define STXXL_VERSION_STRING_PHASE "$(PHASE)"' > reltmp/stxxl-$(REL_VERSION)/common/version.defs
	$(if $(strip $(DATE)),echo '#define STXXL_VERSION_STRING_DATE "$(DATE)"' >> reltmp/stxxl-$(REL_VERSION)/common/version.defs)
	cd reltmp && tar cf - stxxl-$(REL_VERSION) | gzip -9 > ../stxxl-$(REL_VERSION).tar.gz && zip -r ../stxxl-$(REL_VERSION).zip stxxl-$(REL_VERSION)/* 
	$(RM) -r reltmp
	@echo
	@echo "Your release has been created in stxxl-$(REL_VERSION).tar.gz and stxxl-$(REL_VERSION).zip"
	@echo "The following files are modified and not commited:"
	@svn status -q
endif


.PHONY: main config library library-fast tests clean release
