/*
 * Copyright 2021 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/librpitx-C *
 *
 * This is based on other projects:
 *    librpitx (https://github.com/F5OEO/librpitx)
 *        Copyright (C) 2018  Evariste COURJAUD F5OEO
 *
 *    please contact their authors for more information.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef CORE_UTIL_H_
#define CORE_UTIL_H_

#include <stdio.h>
#include <stdarg.h>

void librpitx_dbg_setlevel(int level);
 int librpitx_dbg_getlevel();
void librpitx_dbg_printf(int level, const char *fmt, ...);

#define FREE(x)    librpitx_dbg_printf(2, "->free(" #x ")\n"); \
                   if((x) != NULL) { \
                       free((x)); \
                       x = NULL; \
                   }

#ifdef LIBRPITX_DEBUG
#if LIBRPITX_DEBUG == 1
#define LIBRPITX_DBG_PRINTF(l, fmt, args...)  \
       librpitx_dbg_printf(l, "" fmt, ##args)
#endif
#if LIBRPITX_DEBUG == 2
#define LIBRPITX_DBG_PRINTF(fmt, args...)  \
		librpitx_dbg_printf(l, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#endif
#else
#define LIBRPITX_DBG_PRINTF(l, fmt, args...)
#endif

#endif
