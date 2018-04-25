#ifndef BSD_FUNCS_H
#define BSD_FUNCS_H

#include <string.h>

size_t    strlcpy(char * restrict dst, const char * restrict src, size_t dst_size);
size_t    strlcat(char * restrict dst, const char * restrict src, size_t dst_size);
long long strtonum(const char *numstr, long long minval, long long maxval, const char **errstrp);


#endif /* bsd_funcs.h */
