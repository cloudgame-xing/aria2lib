#define ARIA2_C_API_BUILD
#include "aria2_c_api.h"

#include "../aria2/src/includes/aria2/aria2.h"

int aria2_c_api_init()
{
  return aria2::libraryInit();
}
