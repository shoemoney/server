#ifndef MYSQL_SERVICE_THD_RND_INCLUDED
/* Copyright (C) 2017 MariaDB Corporation

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

/**
  @file
  This service provides access to the thd-local random number generator.

  It's preferable over the global one, because concurrent threads
  can generate random numbers without fighting each other over the access
  to the shared rnd state.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MYSQL_ABI_CHECK
#include <stdlib.h>
#endif

extern struct thd_rnd_service_st {
  double (*thd_rnd_ptr)(MYSQL_THD thd);
  char *(*thd_g_s_n_ptr)(MYSQL_THD thd, size_t n, unsigned char cmin,
                                                  unsigned char cmax);
} *thd_rnd_service;

#ifdef MYSQL_DYNAMIC_PLUGIN
#define thd_rnd(A) thd_rnd_service->thd_rnd_ptr(A)
#define thd_get_session_nonce(A,B,C,D) thd_rnd_service->thd_g_s_n_ptr(A,B,C,D)
#else

double thd_rnd(MYSQL_THD thd);

/**
  Generate string of printable random characters of requested length.

  @param to[out]      Buffer for generation; must be at least length+1 bytes
                      long; result string is always null-terminated
  @param length[in]   How many random characters to put in buffer
*/
char *thd_get_session_nonce(MYSQL_THD thd, size_t n, unsigned char cmin,
                                                     unsigned char cmax);

#endif

#ifdef __cplusplus
}
#endif

#define MYSQL_SERVICE_THD_RND_INCLUDED
#endif
