main: lib

include compiler.make

lib:
	cd io; $(MAKE) lib; cd ..
	cd utils; $(MAKE) create ; cd ..
	echo "Building library is completed."

lib_debug:
	cd io; $(MAKE) lib_debug; cd ..
	echo "Building debug version of the library is completed."

tests: lib
	cd io; $(MAKE); cd ..
	cd mng; $(MAKE); cd ..
	cd containers; $(MAKE); cd ..
	cd algo; $(MAKE); cd ..
	echo "Building tests is completed."
