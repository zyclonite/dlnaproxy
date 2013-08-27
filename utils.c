/* DLNAProxy
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "dlnaproxytypes.h"
#include "getifaddr.h"
#include "upnpglobalvars.h"
#include "log.h"

int
SearchClientCache(struct in_addr addr, int quiet)
{
	int i;
	for( i=0; i<CLIENT_CACHE_SLOTS; i++ )
	{
		if( clients[i].addr.s_addr == addr.s_addr )
		{
			/* Invalidate this client cache if it's older than 1 hour */
			if( (time(NULL) - clients[i].age) > 3600 )
			{
				unsigned char mac[6];
				if( get_remote_mac(addr, mac) == 0 &&
				    memcmp(mac, clients[i].mac, 6) == 0 )
				{
					/* Same MAC as last time when we were able to identify the client,
 					 * so extend the timeout by another hour. */
					clients[i].age = time(NULL);
				}
				else
				{
					memset(&clients[i], 0, sizeof(struct client_cache_s));
					return -1;
				}
			}
			if( !quiet )
				DPRINTF(E_DEBUG, L_HTTP, "Client found in cache. [type %d/entry %d]\n",
					clients[i].type, i);
			return i;
		}
	}
	return -1;
}

inline int
strcatf(struct string_s *str, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vsnprintf(str->data + str->off, str->size - str->off, fmt, ap);
	str->off += ret;
	va_end(ap);

	return ret;
}

/* Find the first occurrence of p in s, where s is terminated by t */
char *
strstrc(const char *s, const char *p, const char t)
{
	char *endptr;
	size_t slen, plen;

	endptr = strchr(s, t);
	if (!endptr)
		return NULL;

	plen = strlen(p);
	slen = endptr - s;
	while (slen >= plen)
	{
		if (*s == *p && strncmp(s+1, p+1, plen-1) == 0)
			return (char*)s;
		s++;
		slen--;
	}

	return NULL;
} 

/* Code basically stolen from busybox */
int
make_dir(char * path, mode_t mode)
{
	char * s = path;
	char c;
	struct stat st;

	do {
		c = '\0';

		/* Bypass leading non-'/'s and then subsequent '/'s. */
		while (*s) {
			if (*s == '/') {
				do {
					++s;
				} while (*s == '/');
				c = *s;     /* Save the current char */
				*s = '\0';     /* and replace it with nul. */
				break;
			}
			++s;
		}

		if (mkdir(path, mode) < 0) {
			/* If we failed for any other reason than the directory
			 * already exists, output a diagnostic and return -1.*/
			if (errno != EEXIST || (stat(path, &st) < 0 || !S_ISDIR(st.st_mode))) {
				DPRINTF(E_WARN, L_GENERAL, "make_dir: cannot create directory '%s'\n", path);
				if (c)
					*s = c;
				return -1;
			}
		}
	        if (!c)
			return 0;

		/* Remove any inserted nul from the path. */
		*s = c;

	} while (1);
}
