/*
Copyright (C) 2014 Eaton
 
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <pthread.h>

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <unistd.h>
# include <sys/syscall.h>
# undef _GNU_SOURCE
#else
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <unistd.h>
# include <sys/syscall.h>
#endif

#include "log.h"
#include "utils.h"

#define ASSERT_LEVEL \
    assert(level == LOG_DEBUG   || \
           level == LOG_INFO    || \
           level == LOG_WARNING || \
           level == LOG_ERR     || \
           level == LOG_CRIT    || \
           level == LOG_NOOP)

#ifdef ENABLE_DEBUG_BUILD
static int log_stderr_level = LOG_DEBUG;
#else
static int log_stderr_level = LOG_WARNING;
#endif
static FILE* log_file = NULL;

extern int errno;

/*XXX: gcc-specific!, see http://stackoverflow.com/questions/7623735/error-initializer-element-is-not-constant */
static void init_log_file(void) __attribute__((constructor(105)));
static void init_log_file(void) {
    log_file = stderr;
}

void log_set_level(int level) {

    ASSERT_LEVEL;

    log_stderr_level = level;
}

int log_get_level() {
    return log_stderr_level;
}

FILE* log_get_file() {
    return log_file;
}

void log_set_file(FILE* file) {
    log_file = file;
}

void log_open() {

    char *ev_log_level = getenv("BIOS_LOG_LEVEL");

    if (ev_log_level) {
        int log_level;
        if (strcmp(ev_log_level, STR(LOG_DEBUG)) == 0) {
            log_level = LOG_DEBUG;
        }
        else if (strcmp(ev_log_level, STR(LOG_INFO)) == 0) {
            log_level = LOG_INFO;
        }
        else if (strcmp(ev_log_level, STR(LOG_WARNING)) == 0) {
            log_level = LOG_WARNING;
        }
        else if (strcmp(ev_log_level, STR(LOG_ERR)) == 0) {
            log_level = LOG_ERR;
        }
        else if (strcmp(ev_log_level, STR(LOG_CRIT)) == 0) {
            log_level = LOG_CRIT;
        } else {
            return;
        }
        log_set_level(log_level);
    }
}

/* Print "PID.PTHREADID" or "PID.PTHREADID.LWPID" on systems
 * where PTHREADID != LWPID into a char* buffer string. This
 * buffer is allocated by asprintf() and freed by caller.
 * Returns pointer to an empty string on errors.
 */
char *
asprintf_thread_id(void) {
    uintmax_t process_id = (uintmax_t)getpid();
    uintmax_t pthread_id = get_current_pthread_id();
    uintmax_t thread_id = get_current_thread_id();
    char *buf = NULL;
    char *buf2 = NULL;

    if (pthread_id != thread_id) {
        if ( 0 > asprintf(&buf2, ".%ju", thread_id) ) {
            if (buf2) {
                free (buf2);
                buf2 = NULL;
            }
        }
    }

    if ( 0 > asprintf(&buf, "%ju.%ju%s",
        process_id, pthread_id, buf2 ? buf2 : ""
    ) ) {
        if (buf) {
            free (buf);
            buf = NULL;
        }
    }

    if (buf2) {
        free (buf2);
        buf2 = NULL;
    }

    if (buf == NULL)
        buf = strdup("");

    return buf;
}

static int do_logv(
        int level,
        const char* file,
        int line,
        const char* func,
        const char* format,
        va_list args) {

    char *prefix;
    char *fmt;
    char *buffer;

    int r;

    if (level > log_get_level()) {
        //no-op if logging disabled
        return 0;
    }

    switch (level) {
        case LOG_DEBUG:
            prefix = "DEBUG"; break;
        case LOG_INFO:
            prefix = "INFO"; break;
        case LOG_WARNING:
            prefix = "WARNING"; break;
        case LOG_ERR:
            prefix = "ERROR"; break;
        case LOG_CRIT:
            prefix = "CRITICAL"; break;
        default:
            fprintf(log_file, "[ERROR]: %s:%d (%s) invalid log level %d", __FILE__, __LINE__, __func__, level);
            return -1;
    };

    char *pidstr = asprintf_thread_id();
    r = asprintf(&fmt, "[%s] [%s]: %s:%d (%s) %s",
        pidstr ? pidstr : "", prefix, file, line, func, format);
    if (pidstr) {
        free(pidstr);
        pidstr = NULL;
    }
    if (r == -1) {
        fprintf(log_file, "[ERROR]: %s:%d (%s) can't allocate enough memory for format string: %m", __FILE__, __LINE__, __func__);
        return r;
    }

    r = vasprintf(&buffer, fmt, args);
    free(fmt);   // we don't need it in any case
    if (r == -1) {
        fprintf(log_file, "[ERROR]: %s:%d (%s) can't allocate enough memory for message string: %m", __FILE__, __LINE__, __func__);
        if (buffer)
            free(buffer);
        return r;
    }

    if (buffer == NULL) {
        fprintf(stderr, "Buffer == NULL\n");
    }
    if (log_file == NULL) {
        fprintf(stderr, "Log_file == NULL\n");
    }
    if (level <= log_stderr_level) {
        fputs(buffer, log_file);
        fputc('\n', log_file);
    }
    free(buffer);

    return 0;
}

int do_log(
        int level,
        const char* file,
        int line,
        const char* func,
        const char* format,
        ...) {

    int r;
    int saved_errno = errno;
    va_list args;

    va_start(args, format);
    r = do_logv(level, file, line, func, format, args);
    va_end(args);

    errno = saved_errno;
    return r;
}

// Return a thread ID number for different platforms. Inspired by:
//   https://issues.apache.org/jira/browse/HADOOP-11638
//   https://github.com/llvm-mirror/llvm/blob/master/lib/Support/Unix/Threading.inc
// Note that the POSIX pthread_t is a generic type which
// may be a structure etc. and is not necessarily same as
// the numeric ID of kernel LWP (which may be the backing
// implementation detail).
// See also:
// https://stackoverflow.com/questions/1759794/how-to-print-pthread-t
// https://stackoverflow.com/questions/14085515/obtain-lwp-id-from-a-pthread-t-on-solaris-to-use-with-processor-bind
// https://stackoverflow.com/questions/34370172/the-thread-id-returned-by-pthread-self-is-not-the-same-thing-as-the-kernel-thr

// This routine returns a pthread_self() casted into a
// numeric value.
uintmax_t
get_current_pthread_id(void)
{
    int sp = sizeof(pthread_t), si = sizeof(uintmax_t);
    uintmax_t thread_id = (uintmax_t)pthread_self();

    if ( sp < si ) {
        // Zero-out extra bits that an uintmax_t value could
        // have populated above the smaller pthread_t value
        int shift_bits = (si - sp) << 3; // * 8
        thread_id = ( (thread_id << shift_bits ) >> shift_bits );
    }

    return thread_id;
}

// This routine aims to return LWP number wherever we
// know how to get its ID.
uintmax_t
get_current_thread_id(void)
{
    // Note: implementations below all assume that the routines
    // return a numeric value castable to uintmax_t, and should
    // fail during compilation otherwise so we can then fix it.
    uintmax_t thread_id = 0;
#if defined(__linux__)
    // returned actual type: long
    thread_id = syscall(SYS_gettid);
#elif defined(__FreeBSD__)
    // returned actual type: int
    thread_id = pthread_getthreadid_np();
#elif defined(__sun)
// TODO: find a way to extract the LWP ID in current PID
// It is known ways exist for Solaris 8-11...
    // returned actual type: pthread_t (which is uint_t in Sol11 at least)
    thread_id = pthread_self();
#elif defined(__APPLE__ )
    pthread_t temp_thread_id;
    (void)pthread_threadid_np(pthread_self(), &temp_thread_id);
    thread_id = temp_thread_id;
#else
#error "Platform not supported: get_current_thread_id() implementation missing"
#endif
    return thread_id;
}
