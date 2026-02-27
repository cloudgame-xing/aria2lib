/* Stub for C++ transaction-clone symbols (GNU TM) from libstdc++.
 * _ZGTtdlPv = transaction-clone of operator delete(void*)
 * _ZGTtnam  = transaction-clone of operator new[](size_t)
 * Forward to the normal operators so the binary has no undefined refs.
 */
#include <cstddef>

#if defined(_WIN32) || defined(__CYGWIN__)
#  define WEAK_ATTR
#else
#  define WEAK_ATTR __attribute__((weak))
#endif

extern "C" {

WEAK_ATTR void _ZGTtdlPv(void* p) {
  ::operator delete(p);
}

WEAK_ATTR void* _ZGTtnam(std::size_t n) {
  return ::operator new[](n);
}

}
