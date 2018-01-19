#/bin/bash

make -j4 || exit -1

ret=0

for t in "syscall unlink" "linuxaio unlink" "memory"; do
   echo -e "\n\n\e[32m\e[21mTest IO subsystem: $t\n========================================================\e[0m\n"
   echo "disk=$STXXL_TMPDIR/stxxl.$$.tmp,256Mi,$t" > ~/.stxxl
   ./tools/stxxl_tool info
   ctest || ret=$?
done

exit $ret
