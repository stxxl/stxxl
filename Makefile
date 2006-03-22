default:
	@echo "Choose one of the goals: library_msvc library_g++ tests_msvc tests_g++ btree_map_test_g++ clean_msvc clean_g++"

settings_msvc:
	copy make.settings.msvc make.settings

settings_g++:
	cp make.settings.g++ make.settings

library_g++: settings_g++
	make -f Makefile.g++ library

library_msvc: settings_msvc
	nmake /F Makefile.msvc library
	
tests_g++: settings_g++
	make -f Makefile.g++ tests

tests_msvc: settings_msvc
	nmake /F Makefile.msvc tests

clean_g++: settings_g++
	make -f Makefile.g++ clean

clean_msvc: settings_msvc
	nmake /F Makefile.msvc clean


