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

# Btree (map) is not yet compatible with g++ 3.4.x and Microsoft
# Visual C++, therefore it is not included into the main tests 
# (the Makefile goal above)
btree_map_test_g++: settings_g++
	make -f Makefile.g++ btree_map_test

clean_g++: settings_g++
	make -f Makefile.g++ clean

clean_msvc: settings_msvc
	nmake /F Makefile.msvc clean


