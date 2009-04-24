# This -*- Makefile -*- gets processed with both GNU make and nmake.
# So keep it simple and compatible.

############################################################################
#  Makefile
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
#  Copyright (C) 2006 Johannes Singler <singler@ira.uka.de>
#  Copyright (C) 2007-2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################


usage:
	@echo "Choose one of the goals:"
	@echo "    config_gnu"
	@echo "    library_g++ library_g++_pmode library_g++_mcstl library_icpc library_icpc_mcstl library_msvc"
	@echo "    tests_g++   tests_g++_pmode   tests_g++_mcstl   tests_icpc   tests_icpc_mcstl   tests_msvc"
	@echo "    clean_g++   clean_g++_pmode   clean_g++_mcstl   clean_icpc   clean_icpc_mcstl   clean_msvc"
	@echo "    doxy clean_doxy"

settings_gnu:
	cmp -s make.settings.gnu make.settings || \
		( $(RM) make.settings && cp make.settings.gnu make.settings )

settings_msvc: make.settings

make.settings: make.settings.msvc
	copy make.settings.msvc make.settings


config_gnu: settings_gnu
	$(MAKE) -f Makefile.gnu config STXXL_AUTOCONFIG=true


library_g++: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_PMODE=no USE_MCSTL=no

library_g++_pmode: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_PMODE=yes USE_MCSTL=no

library_g++_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_PMODE=no USE_MCSTL=yes

library_icpc: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_PMODE=no USE_MCSTL=no USE_ICPC=yes

library_icpc_pmode: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_PMODE=yes USE_MCSTL=no USE_ICPC=yes

library_icpc_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_PMODE=no USE_MCSTL=yes USE_ICPC=yes

library_msvc: settings_msvc
	nmake /NOLOGO /F Makefile.msvc library


tests_g++: library_g++
	$(MAKE) -f Makefile.gnu tests USE_PMODE=no USE_MCSTL=no

tests_g++_pmode: library_g++_pmode
	$(MAKE) -f Makefile.gnu tests USE_PMODE=yes USE_MCSTL=no

tests_g++_mcstl: library_g++_mcstl
	$(MAKE) -f Makefile.gnu tests USE_PMODE=no USE_MCSTL=yes

tests_icpc: library_icpc
	$(MAKE) -f Makefile.gnu tests USE_PMODE=no USE_MCSTL=no USE_ICPC=yes

tests_icpc_pmode: library_icpc_pmode
	$(MAKE) -f Makefile.gnu tests USE_PMODE=yes USE_MCSTL=no USE_ICPC=yes

tests_icpc_mcstl: library_icpc_mcstl
	$(MAKE) -f Makefile.gnu tests USE_PMODE=no USE_MCSTL=yes USE_ICPC=yes

tests_msvc: settings_msvc
	nmake /NOLOGO /F Makefile.msvc tests


clean_g++: settings_gnu
	$(MAKE) -f Makefile.gnu clean USE_PMODE=no USE_MCSTL=no

clean_g++_pmode: settings_gnu
	-$(MAKE) -f Makefile.gnu clean USE_PMODE=yes USE_MCSTL=no

clean_g++_mcstl: settings_gnu
	-$(MAKE) -f Makefile.gnu clean USE_PMODE=no USE_MCSTL=yes

clean_icpc: settings_gnu
	$(MAKE) -f Makefile.gnu clean USE_PMODE=no USE_MCSTL=no USE_ICPC=yes

clean_icpc_pmode: settings_gnu
	-$(MAKE) -f Makefile.gnu clean USE_PMODE=yes USE_MCSTL=no USE_ICPC=yes

clean_icpc_mcstl: settings_gnu
	-$(MAKE) -f Makefile.gnu clean USE_PMODE=no USE_MCSTL=yes USE_ICPC=yes

clean_msvc: settings_msvc
	nmake /NOLOGO /F Makefile.msvc clean


clean: clean_g++ clean_g++_mcstl clean_icpc clean_icpc_mcstl
	$(MAKE) -C test/compile-stxxl-headers clean

distclean: clean_doxy clean_tutorial clean
	$(RM) make.settings
	$(RM) stxxl.log stxxl.errlog
	$(RM) algo/stxxl.log algo/stxxl.errlog
	$(RM) common/stxxl.log common/stxxl.errlog
	$(RM) containers/stxxl.log containers/stxxl.errlog
	$(RM) io/stxxl.log io/stxxl.errlog
	$(RM) mng/stxxl.log mng/stxxl.errlog
	$(RM) stream/stxxl.log stream/stxxl.errlog
	$(RM) utils/stxxl.log utils/stxxl.errlog


doxy: Doxyfile
	doxygen

clean_doxy:
	$(RM) -r doc/doxy
	$(RM) Doxyfile.bak

clean_tutorial:
	$(MAKE) -C doc/tutorial distclean
	$(MAKE) -C doc/tutorial/examples clean

count:
	sloccount --addlang makefile .

# optional parameters:
# DATE=""     if you *don't* want a -YYYYMMDD in the version
# PHASE=snapshot|alpha#|beta#|rc#|release    (defaults to snapshot)
release:
	$(MAKE) -f Makefile.gnu release

