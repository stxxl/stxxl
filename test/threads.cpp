#include <pthread.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#include "../common/myutils.h"


typedef void *(*thread_function_t)(void *);

int nthreads;
pthread_t *threads;
//Switch * switches;
double * ovhtimes;

void * worker(void * arg)
{
  int j=(int) arg;
  ovhtimes[j] = mytimestamp() - ovhtimes[j];
  sleep(3);

  return 0;
}


int main(int argc, char * argv[])
{
  int i=0;
  if(argc < 2)
    {
      cout << "Format: " << argv[0] << " <Threads_number>" << endl;
      exit(-1);
    }

  nthreads = atoi(argv[1]);
  threads = new pthread_t[nthreads];
  //  switches =  new Switch[nthreads];
  ovhtimes = new double[nthreads];
  pthread_attr_t attr;
  mynassert(pthread_attr_init(&attr));
  mynassert(pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM));
  //mynassert(pthread_attr_setscope(&attr,PTHREAD_SCOPE_PROCESS));

  for(i=0;i<nthreads;i++)
    {
      ovhtimes[i]=mytimestamp();
      mynassert(pthread_create(threads + i,&attr,(thread_function_t)worker,(void *)i));
    }

  for(i=0;i<nthreads;i++)
    {
      pthread_join(threads[i],NULL);
    }

  delete []threads;
  //  delete []switches;

  double sum(0.0);
  for(i=0;i<nthreads;i++)
    {
      cout << ovhtimes[i] << " sec."<<endl;
      sum+= ovhtimes[i];
    }
  cout << "Average: " << sum/double(nthreads) << " sec." << endl;


  delete []ovhtimes;

  mynassert(pthread_attr_destroy(&attr));

}


