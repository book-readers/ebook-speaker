/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
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

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <pulse/gccmacro.h>
#include <pulse/volume.h>

#include "i18n.h"
#include "macro.h"
#include "socket.h"

#ifndef PACKAGE
#error "Please include config.h before including this file!"
#endif

struct timeval;

/* These resource limits are pretty new on Linux, let's define them
 * here manually, in case the kernel is newer than the glibc */
#if !defined(RLIMIT_NICE) && defined(__linux__)
#define RLIMIT_NICE 13
#endif
#if !defined(RLIMIT_RTPRIO) && defined(__linux__)
#define RLIMIT_RTPRIO 14
#endif
#if !defined(RLIMIT_RTTIME) && defined(__linux__)
#define RLIMIT_RTTIME 15
#endif

void pa_make_fd_nonblock(int fd);
void pa_make_fd_block(int fd);
bool pa_is_fd_nonblock(int fd);

void pa_make_fd_cloexec(int fd);
ssize_t pa_read(int fd, void *buf, size_t count);
ssize_t pa_loop_read(int fd, void*data, size_t size, int *type);
ssize_t pa_loop_write(int fd, const void*data, size_t size, int *type);

int pa_close(int fd);


char *pa_strlcpy(char *b, const char *s, size_t l);

int pa_parse_boolean(const char *s) PA_GCC_PURE;

int pa_parse_volume(const char *s, pa_volume_t *volume);

char *pa_strip(char *s);

const char *pa_sig2str(int sig) PA_GCC_PURE;
char *pa_hexstr(const uint8_t* d, size_t dlength, char *s, size_t slength);
size_t pa_parsehex(const char *p, uint8_t *d, size_t dlength);

bool pa_startswith (const char *, const char *);
bool pa_endswith   (const char *, const char *);
char *pa_get_binary_name_malloc(void);
char *pa_runtime_path(const char *fn);
char *pa_state_path(const char *fn, bool prepend_machine_id);

int pa_atoi(const char *s, int32_t *ret_i);
int pa_atou(const char *s, uint32_t *ret_u);
int pa_atol(const char *s, long *ret_l);
int pa_atod(const char *s, double *ret_d);

size_t pa_snprintf(char *str, size_t size, const char *format, ...);
size_t pa_vsnprintf(char *str, size_t size, const char *format, va_list ap);

char *pa_truncate_utf8(char *c, size_t l);

char *pa_getcwd(void);
char *pa_make_path_absolute(const char *p);
bool pa_is_path_absolute(const char *p);

void pa_set_env(const char *key, const char *value);
void pa_unset_env(const char *key);


#define pa_streq(a,b) (!strcmp((a),(b)))

/* Like pa_streq, but does not blow up on NULL pointers. */
bool pa_str_in_list_spaces(const char *needle, const char *haystack);

char *pa_get_host_name_malloc(void);
char *pa_get_user_name_malloc(void);

char *pa_machine_id(void);
char *pa_session_id(void);
char *pa_uname_string(void);


unsigned pa_gcd(unsigned a, unsigned b);
void pa_reduce(unsigned *num, unsigned *den);

unsigned pa_ncpus(void);

/* Replaces all occurrences of `a' in `s' with `b'. The caller has to free the
 * returned string. All parameters must be non-NULL and additionally `a' must
 * not be a zero-length string.
 */
char *pa_replace(const char*s, const char*a, const char *b);

/* Escapes p by inserting backslashes in front of backslashes. chars is a
 * regular (i.e. NULL-terminated) string containing additional characters that
 * should be escaped. chars can be NULL. The caller has to free the returned
 * string. */
char *pa_escape(const char *p, const char *chars);

/* Does regular backslash unescaping. Returns the argument p. */
char *pa_unescape(char *p);

char *pa_realpath(const char *path);

void pa_disable_sigpipe(void);

void pa_xfreev(void**a);

char* pa_maybe_prefix_path(const char *path, const char *prefix);

/* Returns size of the specified pipe or 4096 on failure */
size_t pa_pipe_buf(int fd);
size_t pa_page_size(void);

/* Rounds down */
static inline void* PA_PAGE_ALIGN_PTR(const void *p) {
    return (void*) (((size_t) p) & ~(pa_page_size() - 1));
}

/* Rounds up */
static inline size_t PA_PAGE_ALIGN(size_t l) {
    size_t page_size = pa_page_size();
    return (l + page_size - 1) & ~(page_size - 1);
}
