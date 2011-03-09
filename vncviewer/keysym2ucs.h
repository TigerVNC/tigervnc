/*
 * This module converts keysym values into the corresponding ISO 10646-1
 * (UCS, Unicode) values.
 */

#ifdef __cplusplus
extern "C" {
#endif

unsigned keysym2ucs(unsigned keysym);
unsigned ucs2keysym(unsigned ucs);

#ifdef __cplusplus
}
#endif
