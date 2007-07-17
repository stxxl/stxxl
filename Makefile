# This -*- Makefile -*- gets processed with both GNU make and nmake.
# So keep it simple and compatible.

usage:
	@echo "Choose one of the goals:"
	@echo "    library_g++ library_g++_mcstl library_icpc library_icpc_mcstl library_msvc"
	@echo "    tests_g++   tests_g++_mcstl   tests_icpc   tests_icpc_mcstl   tests_msvc"
	@echo "    clean_g++   clean_g++_mcstl   clean_icpc   clean_icpc_mcstl   clean_msvc"
	@echo "    doxy clean_doxy"

settings_gnu:
	cmp -s make.settings.gnu make.settings || \
		cp make.settings.gnu make.settings

settings_msvc:
	copy make.settings.msvc make.settings


library_g++: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_MCSTL=no

library_g++_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_MCSTL=yes

library_icpc: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_MCSTL=no USE_ICPC=yes

library_icpc_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu library USE_MCSTL=yes USE_ICPC=yes

library_msvc: settings_msvc
	nmake /F Makefile.msvc library
	

tests_g++: settings_gnu
	$(MAKE) -f Makefile.gnu tests USE_MCSTL=no

tests_g++_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu tests USE_MCSTL=yes

tests_icpc: settings_gnu
	$(MAKE) -f Makefile.gnu tests USE_MCSTL=no USE_ICPC=yes

tests_icpc_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu tests USE_MCSTL=yes USE_ICPC=yes

tests_msvc: settings_msvc
	nmake /F Makefile.msvc tests


clean_g++: settings_gnu
	$(MAKE) -f Makefile.gnu clean USE_MCSTL=no

clean_g++_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu clean USE_MCSTL=yes

clean_icpc: settings_gnu
	$(MAKE) -f Makefile.gnu clean USE_MCSTL=no USE_ICPC=yes

clean_icpc_mcstl: settings_gnu
	$(MAKE) -f Makefile.gnu clean USE_MCSTL=yes USE_ICPC=yes

clean_msvc: settings_msvc
	nmake /F Makefile.msvc clean

doxy: Doxyfile
	doxygen

clean_doxy:
	$(RM) -r doc/doxy

