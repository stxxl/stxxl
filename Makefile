main: lib

include compiler.make

lib:
	cd io; $(MAKE) lib; cd ..
	echo "Building library is completed."

tests: lib
	cd io; $(MAKE); cd ..
	cd mng; $(MAKE); cd ..
	cd containers; $(MAKE); cd ..
	cd algo; $(MAKE); cd ..
	echo "Building tests is completed."
