/*
 * These functions are taken from FreeBSD's libc. They are provided as libbsd
 * on linux, but are non-standard enough that it is convenient to just include
 * them in source form. These are almost verbatim copies, I only changed a few
 * variable names I thought were a bit too short (osrc to orig_src, etc). The
 * copyright notices were copied entirely unchanged.
 */

#include "bsd_funcs.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/*============================================================================*/
/*============================================================================*/
/* strlcpy */

/* $OpenBSD: strlcpy.c,v 1.12 2015/01/15 03:54:12 millert Exp $ */
/*
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * Copy string src to buffer dst of size dst_size.  At most dst_size-1
 * chars will be copied.  Always NUL terminates (unless dst_size == 0).
 * Returns strlen(src); if retval >= dst_size, truncation occurred.
 */
size_t
neotags_strlcpy(char * __restrict dst, const char * __restrict src, size_t dst_size)
{
        const char *orig_src = src;
        size_t nleft = dst_size;

        /* Copy as many bytes as will fit */
        if (nleft != 0)
                while (--nleft != 0)  /* Break when nleft is 1 */
                        if ((*dst++ = *src++) == '\0')
                                break;

        /* If not enough room in dst, add NUL and traverse rest of src to find
         * its remaining length. */
        if (nleft == 0) {
                if (dst_size != 0)
                        *dst = '\0';  /* NUL-terminate dst */
                while (*src++ != '\0')
                        ;
        }

        return (src - orig_src - 1);  /* count does not include NUL */
}

/*============================================================================*/
/*============================================================================*/
/* strlcat */

/* $OpenBSD: strlcat.c,v 1.15 2015/03/02 21:41:08 millert Exp $ */
/*
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size dst_size (unlike strncat, dst_size is the
 * full size of dst, not space left).  At most dst_size-1 characters
 * will be copied.  Always NUL terminates (unless dst_size <= strlen(dst)).
 * Returns strlen(src) + MIN(dst_size, strlen(initial dst)).
 * If retval >= dst_size, truncation occurred.
 */
size_t
neotags_strlcat(char * __restrict dst, const char * __restrict src, size_t dst_size)
{
        const char *orig_dst = dst;
        const char *orig_src = src;
        size_t nleft = dst_size;
        size_t dst_len;

        /* Find the end of dst and adjust bytes left but don't go past end. */
        while (nleft-- != 0 && *dst != '\0')
                ++dst;
        dst_len = dst - orig_dst;
        nleft = dst_size - dst_len;

        if (nleft-- == 0)
                return (dst_len + strlen(src));

        while (*src != '\0') {
                if (nleft != 0) {
                        *dst++ = *src;
                        nleft--;
                }
                ++src;
        }
        *dst = '\0';

        return (dst_len + (src - orig_src));  /* count does not include NUL */
}

/*============================================================================*/
/*============================================================================*/
/* strtonum */

/*-
 * Copyright (c) 2004 Ted Unangst and Todd Miller
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *      $OpenBSD: strtonum.c,v 1.7 2013/04/17 18:40:58 tedu Exp $
 */

#define INVALID  1
#define TOOSMALL 2
#define TOOLARGE 3

long long
neotags_strtonum(const char *numstr,
         long long minval,
         long long maxval,
         const char **errstrp)
{
        long long ret = 0;
        int error = 0;
        char *ep;
        struct errval {
                const char *errstr;
                int err;
        } ev[4] = {
                { NULL,         0 },
                { "invalid",    EINVAL },
                { "too small",  ERANGE },
                { "too large",  ERANGE },
        };

        ev[0].err = errno;
        errno = 0;
        if (minval > maxval) {
                error = INVALID;
        } else {
                ret = strtoll(numstr, &ep, 10);
                if (errno == EINVAL || numstr == ep || *ep != '\0')
                        error = INVALID;
                else if ((ret == LLONG_MIN && errno == ERANGE) || ret < minval)
                        error = TOOSMALL;
                else if ((ret == LLONG_MAX && errno == ERANGE) || ret > maxval)
                        error = TOOLARGE;
        }
        if (errstrp != NULL)
                *errstrp = ev[error].errstr;
        errno = ev[error].err;
        if (error)
                ret = 0;

        return (ret);
}


#ifndef HAVE_STRSEP
/*============================================================================*/
/*============================================================================*/
/* strsep */

/*-
* SPDX-License-Identifier: BSD-3-Clause
*
* Copyright (c) 1990, 1993
*	The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

/*
 * NOTE: Unlike GNU strsep, FreeBSD's strsep only looks for the first
 *       character in the `delim' string.
 */
char *
strsep(char **stringp, const char *delim)
{
        const char *delimp;
        char *ptr, *tok;
        char src_ch, del_ch;

        if ((ptr = tok = *stringp) == NULL)
                return (NULL);

        for (;;) {
                src_ch = *ptr++;
                delimp = delim;
                do {
                        if ((del_ch = *delimp++) == src_ch) {
                                if (src_ch == '\0')
                                        ptr = NULL;
                                else
                                        ptr[-1] = '\0';
                                *stringp = ptr;
                                return (tok);
                        }
                } while (del_ch != '\0');
        }
        /* NOTREACHED */
}
#endif

