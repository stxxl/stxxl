all: kmain

##GCCVER = -V 2.95.3
##GCCVER = -V 3.0.4
GCCVER = 

MAKE = gmake
##MAKE = mp
##GCC = $(PURIFY) -program-name=/ZB/IMPRS/rdementi/projects/STXXL/apps/sort  g++ $(GCCVER) -Wall -g 
##GCC=g++ $(GCCVER) -Wall -O2 -pg
GCC=icc $(GCCVER) -Wall -O3 -xK 

##GCC=g++ $(GCCVER) -Wall -O3 
##GCC=g++ $(GCCVER) -Wall -g 

l1: 
	$(GCC) radixsort.C -o l1sort

l2: 
	$(GCC) radixsort2.C -o l2sort 

clean:
	rm -f *.o sort 
