/*************************************************************************************************
 * System-dependent configurations
 *                                                               Copyright (C) 2009-2012 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#ifndef _MYCONF_H                        // duplication check
#define _MYCONF_H



/*************************************************************************************************
 * system discrimination
 *************************************************************************************************/


#if defined(__linux__)

#define _SYS_LINUX_
#define _KC_OSNAME     "Linux"

#elif defined(__FreeBSD__)

#define _SYS_FREEBSD_
#define _KC_OSNAME     "FreeBSD"

#elif defined(__NetBSD__)

#define _SYS_NETBSD_
#define _KC_OSNAME     "NetBSD"

#elif defined(__OpenBSD__)

#define _SYS_OPENBSD_
#define _KC_OSNAME     "OpenBSD"

#elif defined(__sun__) || defined(__sun)

#define _SYS_SUNOS_
#define _KC_OSNAME     "SunOS"

#elif defined(__hpux)

#define _SYS_HPUX_
#define _KC_OSNAME     "HP-UX"

#elif defined(__osf)

#define _SYS_TRU64_
#define _KC_OSNAME     "Tru64"

#elif defined(_AIX)

#define _SYS_AIX_
#define _KC_OSNAME     "AIX"

#elif defined(__APPLE__) && defined(__MACH__)

#define _SYS_MACOSX_
#define _KC_OSNAME     "Mac OS X"

#elif defined(_MSC_VER)

#define _SYS_MSVC_
#define _KC_OSNAME     "Windows (VC++)"

#elif defined(_WIN32)

#define _SYS_MINGW_
#define _KC_OSNAME     "Windows (MinGW)"

#elif defined(__CYGWIN__)

#define _SYS_CYGWIN_
#define _KC_OSNAME     "Windows (Cygwin)"

#else

#define _SYS_GENERIC_
#define _KC_OSNAME     "Generic"

#endif

#define _KC_VERSION    "1.2.76"
#define _KC_LIBVER     16
#define _KC_LIBREV     13
#define _KC_FMTVER     5

#if defined(_MYBIGEND)
#define _KC_BIGEND     1
#else
#define _KC_BIGEND     0
#endif

#if defined(_MYGCCATOMIC)
#define _KC_GCCATOMIC  1
#else
#define _KC_GCCATOMIC  0
#endif

#if defined(_MYZLIB)
#define _KC_ZLIB       1
#else
#define _KC_ZLIB       0
#endif

#if defined(_MYLZO)
#define _KC_LZO        1
#else
#define _KC_LZO        0
#endif

#if defined(_MYLZMA)
#define _KC_LZMA       1
#else
#define _KC_LZMA       0
#endif

#if defined(_SYS_MSVC_)
#define _KC_PXREGEX    0
#else
#define _KC_PXREGEX    1
#endif



/*************************************************************************************************
 * notation of the file system
 *************************************************************************************************/


#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)

#define MYPATHCHR      '\\'
#define MYPATHSTR      "\\"
#define MYEXTCHR       '.'
#define MYEXTSTR       "."
#define MYCDIRSTR      "."
#define MYPDIRSTR      ".."

#else

#define MYPATHCHR      '/'
#define MYPATHSTR      "/"
#define MYEXTCHR       '.'
#define MYEXTSTR       "."
#define MYCDIRSTR      "."
#define MYPDIRSTR      ".."

#endif



/*************************************************************************************************
 * general headers
 *************************************************************************************************/


extern "C" {
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
}

extern "C" {
#include <stdint.h>
}

#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)

#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <process.h>

#else

extern "C" {
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <dirent.h>
}

extern "C" {
#include <pthread.h>
#include <sched.h>
}

#endif

#if defined(_SYS_FREEBSD_) || defined(_SYS_OPENBSD_) || defined(_SYS_NETBSD_) || \
  defined(_SYS_MACOSX_)
#define pthread_spinlock_t       pthread_mutex_t
#define pthread_spin_init(KC_a, KC_b)           \
  pthread_mutex_init(KC_a, NULL)
#define pthread_spin_destroy(KC_a)              \
  pthread_mutex_destroy(KC_a)
#define pthread_spin_lock(KC_a)                 \
  pthread_mutex_lock(KC_a)
#define pthread_spin_trylock(KC_a)              \
  pthread_mutex_trylock(KC_a)
#define pthread_spin_unlock(KC_a)               \
  pthread_mutex_unlock(KC_a)
#endif


#endif                                   // duplication check


// END OF FILE
