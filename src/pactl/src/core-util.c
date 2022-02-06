/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2004 Joe Marcus Clarke
  Copyright 2006-2007 Pierre Ossman <ossman@cendio.se> for Cendio AB

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with PulseAudio; if not, see <http://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pulse/xmalloc.h>
#include <pulse/util.h>
#include <pulse/utf8.h>

#include "core-error.h"
#include "socket.h"
#include "macro.h"
#include "strbuf.h"
#include "strlist.h"
#include "core-util.h"

/* Similar to OpenBSD's strlcpy() function */
char *pa_strlcpy(char *b, const char *s, size_t l) {
    size_t k;

    pa_assert(b);
    pa_assert(s);
    pa_assert(l > 0);

    k = strlen(s);

    if (k > l-1)
        k = l-1;

    memcpy(b, s, k);
    b[k] = 0;

    return b;
}
#ifdef HAVE_SYS_RESOURCE_H
static int set_nice(int nice_level) {

#ifdef HAVE_SYS_RESOURCE_H
    if (setpriority(PRIO_PROCESS, 0, nice_level) >= 0) {
        return 0;
    }
#endif
    return -1;
}
#endif

/* Try to parse a boolean string value. */
int pa_parse_boolean(const char *v) {
    pa_assert(v);

    /* First we check language independent */
    if (pa_streq(v, "1") || !strcasecmp(v, "y") || !strcasecmp(v, "t")
            || !strcasecmp(v, "yes") || !strcasecmp(v, "true") || !strcasecmp(v, "on"))
        return 1;
    else if (pa_streq(v, "0") || !strcasecmp(v, "n") || !strcasecmp(v, "f")
                 || !strcasecmp(v, "no") || !strcasecmp(v, "false") || !strcasecmp(v, "off"))
        return 0;


    errno = EINVAL;
    return -1;
}

/* Try to parse a volume string to pa_volume_t. The allowed formats are:
 * db, % and unsigned integer */
int pa_parse_volume (const char *v, pa_volume_t *volume)
{
    int len;
    uint32_t i;
    double d;
    char str[64];

    pa_assert(v);
    pa_assert(volume);

    len = (int) strlen(v);

    if (len >= 64)
        return -1;

    memcpy(str, v, (size_t) len + 1);

    if (str[len - 1] == '%') {
        str[len - 1] = '\0';
        if (pa_atod(str, &d) < 0)
            return -1;

        d = d / 100 * PA_VOLUME_NORM;

        if (d < 0 || d > PA_VOLUME_MAX)
            return -1;

        *volume = (pa_volume_t) d;
        return 0;
    }

    if (pa_atou (v, &i) < 0 || ! PA_VOLUME_IS_VALID (i))
        return -1;

    *volume = i;
    return 0;
}

/* Convert the string s to an unsigned integer in *ret_u */
int pa_atou(const char *s, uint32_t *ret_u) {
    char *x = NULL;
    unsigned long l;

    pa_assert(s);
    pa_assert(ret_u);

    /* strtoul() ignores leading spaces. We don't. */
    if (isspace((unsigned char)*s)) {
        errno = EINVAL;
        return -1;
    }

    /* strtoul() accepts strings that start with a minus sign. In that case the
     * original negative number gets negated, and strtoul() returns the negated
     * result. We don't want that kind of behaviour. strtoul() also allows a
     * leading plus sign, which is also a thing that we don't want. */
    if (*s == '-' || *s == '+') {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    l = strtoul(s, &x, 0);

    /* If x doesn't point to the end of s, there was some trailing garbage in
     * the string. If x points to s, no conversion was done (empty string). */
    if (!x || *x || x == s || errno) {
        if (!errno)
            errno = EINVAL;
        return -1;
    }

    if ((uint32_t) l != l) {
        errno = ERANGE;
        return -1;
    }

    *ret_u = (uint32_t) l;

    return 0;
}

/* Convert the string s to a signed long integer in *ret_l. */
int pa_atol(const char *s, long *ret_l) {
    char *x = NULL;
    long l;

    pa_assert(s);
    pa_assert(ret_l);

    /* strtol() ignores leading spaces. We don't. */
    if (isspace((unsigned char)*s)) {
        errno = EINVAL;
        return -1;
    }

    /* strtol() accepts leading plus signs, but that's ugly, so we don't allow
     * that. */
    if (*s == '+') {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    l = strtol(s, &x, 0);

    /* If x doesn't point to the end of s, there was some trailing garbage in
     * the string. If x points to s, no conversion was done (at least an empty
     * string can trigger this). */
    if (!x || *x || x == s || errno) {
        if (!errno)
            errno = EINVAL;
        return -1;
    }

    *ret_l = l;

    return 0;
}

#ifdef HAVE_STRTOD_L
static locale_t c_locale = NULL;

static void c_locale_destroy(void) {
    freelocale(c_locale);
}
#endif

int pa_atod(const char *s, double *ret_d) {
    char *x = NULL;
    double f;

    pa_assert(s);
    pa_assert(ret_d);

    /* strtod() ignores leading spaces. We don't. */
    if (isspace((unsigned char)*s)) {
        errno = EINVAL;
        return -1;
    }

    /* strtod() accepts leading plus signs, but that's ugly, so we don't allow
     * that. */
    if (*s == '+') {
        errno = EINVAL;
        return -1;
    }

    /* This should be locale independent */

#ifdef HAVE_STRTOD_L

    PA_ONCE_BEGIN {

        if ((c_locale = newlocale(LC_ALL_MASK, "C", NULL)))
            atexit(c_locale_destroy);

    } PA_ONCE_END;

    if (c_locale) {
        errno = 0;
        f = strtod_l(s, &x, c_locale);
    } else
#endif
    {
        errno = 0;
        f = strtod(s, &x);
    }

    /* If x doesn't point to the end of s, there was some trailing garbage in
     * the string. If x points to s, no conversion was done (at least an empty
     * string can trigger this). */
    if (!x || *x || x == s || errno) {
        if (!errno)
            errno = EINVAL;
        return -1;
    }

    if (isnan(f)) {
        errno = EINVAL;
        return -1;
    }

    *ret_d = f;

    return 0;
}

/* Same as snprintf, but guarantees NUL-termination on every platform */
size_t pa_snprintf(char *str, size_t size, const char *format, ...) {
    size_t ret;
    va_list ap;

    pa_assert(str);
    pa_assert(size > 0);
    pa_assert(format);

    va_start(ap, format);
    ret = pa_vsnprintf(str, size, format, ap);
    va_end(ap);

    return ret;
}

/* Same as vsnprintf, but guarantees NUL-termination on every platform */
size_t pa_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    int ret;

    pa_assert(str);
    pa_assert(size > 0);
    pa_assert(format);

    ret = vsnprintf(str, size, format, ap);

    str[size-1] = 0;

    if (ret < 0)
        return strlen(str);

    if ((size_t) ret > size-1)
        return size-1;

    return (size_t) ret;
}

/* Truncate the specified string, but guarantee that the string
 * returned still validates as UTF8 */
char *pa_truncate_utf8(char *c, size_t l) {
    pa_assert(c);
    pa_assert(pa_utf8_valid(c));

    if (strlen(c) <= l)
        return c;

    c[l] = 0;

    while (l > 0 && !pa_utf8_valid(c))
        c[--l] = 0;

    return c;
}

char *pa_getcwd(void) {
    size_t l = 128;

    for (;;) {
        char *p = pa_xmalloc(l);
        if (getcwd(p, l))
            return p;

        if (errno != ERANGE) {
            pa_xfree(p);
            return NULL;
        }

        pa_xfree(p);
        l *= 2;
    }
}

void pa_set_env(const char *key, const char *value) {
    pa_assert(key);
    pa_assert(value);

    /* This is not thread-safe */

    setenv(key, value, 1);
}

void pa_unset_env(const char *key) {
    pa_assert(key);

    /* This is not thread-safe */

    unsetenv(key);
}

char *pa_get_user_name_malloc(void) {
    ssize_t k;
    char *u;

#ifdef _SC_LOGIN_NAME_MAX
    k = (ssize_t) sysconf(_SC_LOGIN_NAME_MAX);

    if (k <= 0)
#endif
        k = 32;

    u = pa_xnew (char, (size_t) k + 1);

    if (!(pa_get_user_name(u, (size_t) k))) {
        pa_xfree(u);
        return NULL;
    }

    return u;
}

char *pa_get_host_name_malloc(void) {
    size_t l;

    l = 100;
    for (;;) {
        char *c;

        c = pa_xmalloc(l);

        if (!pa_get_host_name(c, l)) {

            if (errno != EINVAL && errno != ENAMETOOLONG)
                break;

        } else if (strlen(c) < l-1) {
            char *u;

            if (*c == 0) {
                pa_xfree(c);
                break;
            }

            u = pa_utf8_filter(c);
            pa_xfree(c);
            return u;
        }

        /* Hmm, the hostname is as long the space we offered the
         * function, we cannot know if it fully fit in, so let's play
         * safe and retry. */

        pa_xfree(c);
        l *= 2;
    }

    return NULL;
}

char *pa_session_id(void) {
    const char *e;

    e = getenv("XDG_SESSION_ID");
    if (!e)
        return NULL;

    return pa_utf8_filter(e);
}

unsigned pa_gcd(unsigned a, unsigned b) {

    while (b > 0) {
        unsigned t = b;
        b = a % b;
        a = t;
    }

    return a;
}

void pa_reduce(unsigned *num, unsigned *den) {

    unsigned gcd = pa_gcd(*num, *den);

    if (gcd <= 0)
        return;

    *num /= gcd;
    *den /= gcd;

    pa_assert(pa_gcd(*num, *den) == 1);
}

unsigned pa_ncpus(void) {
    long ncpus;

#ifdef _SC_NPROCESSORS_ONLN
    ncpus = sysconf(_SC_NPROCESSORS_ONLN);
#else
    ncpus = 1;
#endif

    return ncpus <= 0 ? 1 : (unsigned) ncpus;
}

char *pa_replace(const char*s, const char*a, const char *b) {
    pa_strbuf *sb;
    size_t an;

    pa_assert(s);
    pa_assert(a);
    pa_assert(*a);
    pa_assert(b);

    an = strlen(a);
    sb = pa_strbuf_new();

    for (;;) {
        const char *p;

        if (!(p = strstr(s, a)))
            break;

        pa_strbuf_putsn(sb, s, (size_t) (p - s));
        pa_strbuf_puts(sb, b);
        s = p + an;
    }

    pa_strbuf_puts(sb, s);

    return pa_strbuf_to_string_free(sb);
}

char *pa_escape(const char *p, const char *chars) {
    const char *s;
    const char *c;
    pa_strbuf *buf = pa_strbuf_new();

    for (s = p; *s; ++s) {
        if (*s == '\\')
            pa_strbuf_putc(buf, '\\');
        else if (chars) {
            for (c = chars; *c; ++c) {
                if (*s == *c) {
                    pa_strbuf_putc(buf, '\\');
                    break;
                }
            }
        }
        pa_strbuf_putc(buf, *s);
    }

    return pa_strbuf_to_string_free(buf);
}

char *pa_unescape(char *p) {
    char *s, *d;
    bool escaped = false;

    for (s = p, d = p; *s; s++) {
        if (!escaped && *s == '\\') {
            escaped = true;
            continue;
        }

        *(d++) = *s;
        escaped = false;
    }

    *d = 0;

    return p;
}

char *pa_realpath(const char *path) {
    char *t;
    pa_assert(path);

    /* We want only absolute paths */
    if (path[0] != '/') {
        errno = EINVAL;
        return NULL;
    }

#if defined(__GLIBC__)
    {
        char *r;

        if (!(r = realpath(path, NULL)))
            return NULL;

        /* We copy this here in case our pa_xmalloc() is not
         * implemented on top of libc malloc() */
        t = pa_xstrdup(r);
        pa_xfree(r);
    }
#elif defined(PATH_MAX)
    {
        char *path_buf;
        path_buf = pa_xmalloc(PATH_MAX);

        if (!(t = realpath(path, path_buf))) {
            pa_xfree(path_buf);
            return NULL;
        }
    }
#else
#error "It's not clear whether this system supports realpath(..., NULL) like GNU libc does. If it doesn't we need a private version of realpath() here."
#endif

    return t;
}

void pa_disable_sigpipe(void) {

#ifdef SIGPIPE
    struct sigaction sa;

    pa_zero(sa);

    if (sigaction(SIGPIPE, NULL, &sa) < 0) {
        return;
    }

    sa.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        return;
    }
#endif
}

void pa_xfreev(void**a) {
    void **p;

    if (!a)
        return;

    for (p = a; *p; p++)
        pa_xfree(*p);

    pa_xfree(a);
}

size_t pa_page_size(void) {
#if defined(PAGE_SIZE)
    return PAGE_SIZE;
#elif defined(PAGESIZE)
    return PAGESIZE;
#elif defined(HAVE_SYSCONF)
    static size_t page_size = 4096; /* Let's hope it's like x86. */

    PA_ONCE_BEGIN {
        long ret = sysconf(_SC_PAGE_SIZE);
        if (ret > 0)
            page_size = ret;
    } PA_ONCE_END;

    return page_size;
#else
    return 4096;
#endif
}

/* Returns nonzero when *s ends with *sfx */
bool pa_endswith(const char *s, const char *sfx) {
    size_t l1, l2;

    pa_assert(s);
    pa_assert(sfx);

    l1 = strlen(s);
    l2 = strlen(sfx);

    return l1 >= l2 && pa_streq(s + l1 - l2, sfx);
}

/* Returns nonzero when *s starts with *pfx */
bool pa_startswith (const char *s, const char *pfx)
{
    size_t l;

    pa_assert(s);
    pa_assert(pfx);

    l = strlen(pfx);

    return strlen(s) >= l && strncmp(s, pfx, l) == 0;
}
