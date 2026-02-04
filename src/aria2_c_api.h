#ifndef ARIA2_C_API_H
#define ARIA2_C_API_H

#if defined(_WIN32)
#  if defined(ARIA2_C_API_BUILD)
#    define ARIA2_C_API __declspec(dllexport)
#  else
#    define ARIA2_C_API __declspec(dllimport)
#  endif
#else
#  define ARIA2_C_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

ARIA2_C_API int aria2_c_api_init();

#ifdef __cplusplus
}
#endif

#endif
