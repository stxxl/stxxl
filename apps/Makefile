all: main


include ../compiler.make

main: msort
##	$(MAKE) -C ../IO 
	$(GCC) msort.o ../io/diskqueue.o ../io/mc.o -o sort -lpthread

kmain: mksort
	$(GCC) mkeysort.o ../io/diskqueue.o ../io/mc.o -o ksort -lpthread

main2: msort
	$(GCC) msort.o diskqueue.o mc.o -o sort -lpthread

msort:
	$(GCC) -c msort.cpp

mksort:
	$(GCC) -c mkeysort.cpp

clean:
	rm -f *.o sort 
