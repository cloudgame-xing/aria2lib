#include <iostream>

#include "aria2_c_api.h"

int main()
{
  const int init_result = aria2_c_api_init();
  std::cout << "aria2 init result: " << init_result << std::endl;
  return 0;
}
