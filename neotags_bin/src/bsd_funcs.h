#ifndef BSD_FUNCS_H
#define BSD_FUNCS_H

#ifdef __cplusplus
   extern "C" {
#endif

#include <string.h>
#ifndef __GNUC__
#  define restrict __restrict
#endif

size_t    strlcpy(char * restrict dst, const char * restrict src, size_t dst_size);
size_t    strlcat(char * restrict dst, const char * restrict src, size_t dst_size);
long long strtonum(const char *numstr, long long minval, long long maxval, const char **errstrp);


#ifdef __cplusplus
   }
#endif

#endif /* bsd_funcs.h */
