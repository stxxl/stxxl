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
	cd stream; $(MAKE); cd ..
	echo "Building tests is completed."
	
clean:
	cd io; $(MAKE) clean; cd ..
	cd mng; $(MAKE) clean; cd ..
	cd containers; $(MAKE) clean; cd ..
	cd algo; $(MAKE) clean; cd ..
	cd stream; $(MAKE) clean; cd ..
	echo "Cleaning completed"
