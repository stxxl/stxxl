default:
	@echo -e "Choose one of the goals:\nlibrary_g++ library_g++_mcstl library_icpc library_icpc_mcstl library_msvc\ntests_g++ tests_g++_mcstl tests_icpc tests_icpc_mcstl tests_msvc\nclean_g++ clean_g++_mcstl clean_icpc clean_icpc_mcstl clean_msvc "

settings_g++:
	cp make.settings.g++ make.settings

settings_g++_mcstl:
	cp make.settings.g++_mcstl make.settings

settings_icpc:
	cp make.settings.icpc make.settings

settings_icpc_mcstl:
	cp make.settings.icpc_mcstl make.settings

settings_msvc:
	copy make.settings.msvc make.settings


library_g++: settings_g++
	$(MAKE) -f Makefile.g++ library

library_g++_mcstl: settings_g++_mcstl
	$(MAKE) -f Makefile.g++ library

library_icpc: settings_icpc
	$(MAKE) -f Makefile.icpc library

library_icpc_mcstl: settings_icpc_mcstl
	$(MAKE) -f Makefile.icpc library

library_msvc: settings_msvc
	nmake /F Makefile.msvc library
	

tests_g++: settings_g++
	$(MAKE) -f Makefile.g++ tests

tests_g++_mcstl: settings_g++_mcstl
	$(MAKE) -f Makefile.g++ tests

tests_icpc: settings_icpc
	$(MAKE) -f Makefile.icpc tests

tests_icpc_mcstl: settings_icpc_mcstl
	$(MAKE) -f Makefile.icpc tests

tests_msvc: settings_msvc
	nmake /F Makefile.msvc tests


clean_g++: settings_g++
	$(MAKE) -f Makefile.g++ clean

clean_g++_mcstl: settings_g++_mcstl
	$(MAKE) -f Makefile.g++ clean

clean_icpc: settings_icpc
	$(MAKE) -f Makefile.icpc clean

clean_icpc_mcstl: settings_icpc_mcstl
	$(MAKE) -f Makefile.icpc clean

clean_msvc: settings_msvc
	nmake /F Makefile.msvc clean


