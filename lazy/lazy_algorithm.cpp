#include "algorithm.h"
#include <iostream>
#include <vector>

using namespace std;

int main()
{
  vector<int> input(100);
  vector<int> output(100);
  
  fill(input.begin(),input.end(),555);
  fill(output.begin(),output.end(),333);
  
  iterator_range_to_la<vector<int>::iterator> 
    lazy_input = create_lazy_stream(input.begin(),input.end()); // creating input stream
  
  
  
  
  flush_to_iterator(output.begin(),lazy_input); // flushing lazy stream to container
  
  return 0;
}
