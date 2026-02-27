/* Stub for GNU Transactional Memory (ITM) symbols from libstdc++.
 * These are weak refs when libstdc++ is built with TM support; providing
 * no-op definitions lets the linker resolve them so they are not imported.
 * Not used at runtime by aria2.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  define WEAK_ATTR
#else
#  define WEAK_ATTR __attribute__((weak))
#endif

WEAK_ATTR void _ITM_addUserCommitAction(void) {}
WEAK_ATTR void _ITM_memcpyRtWn(void) {}
WEAK_ATTR void _ITM_RU1(void) {}
WEAK_ATTR void _ITM_deregisterTMCloneTable(void) {}
WEAK_ATTR void _ITM_registerTMCloneTable(void) {}
WEAK_ATTR void _ITM_RU8(void) {}
WEAK_ATTR void _ITM_memcpyRnWt(void) {}

#if defined(__cplusplus)
}
#endif
