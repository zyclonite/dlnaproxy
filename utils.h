/* Utility functions
 *
 * Project : dlnaproxy
 * Author  : Lukas Prettenthaler
 *
 * DLNA announcement proxy
 * Copyright (C) 2011  Lukas Prettenthaler
 *
 * This file is part of DLNAProxy.
 *
 * DLNAProxy is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * DLNAProxy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DLNAProxy. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __UTILS_H__
#define __UTILS_H__

int
SearchClientCache(struct in_addr addr, int quiet);

int
strcatf(struct string_s *str, char *fmt, ...);

char *
strstrc(const char *s, const char *p, const char t);

int
make_dir(char * path, mode_t mode);

#endif
