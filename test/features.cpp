#include <iostream>

int main()
{

#ifdef _POSIX_SYNCHRONIZED_IO
  cout << "_POSIX_SYNCHRONIZED_IO " << endl;
#endif

#ifdef _POSIX_FSYNC
    cout << "_POSIX_FSYNC" << endl;
#endif

#ifdef _POSIX_SYNC_IO
    cout << " _POSIX_SYNC_IO" <<endl;
#endif

#ifdef _POSIX_THREADS
    cout << " _POSIX_THREADS " << endl;
#endif

  return 0;
}
