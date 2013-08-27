/* DLNAProxy project
 *
 * DLNA announcement proxy
 * Copyright (C) 2011  Lukas Prettenthaler
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
 *
 * Portions of the code from the MiniUPnP project:
 *
 * Copyright (c) 2006-2007, Thomas Bernard
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 #ifndef __UPNPGLOBALVARS_H__
#define __UPNPGLOBALVARS_H__

#include <time.h>

#include "dlnaproxytypes.h"
#include "config.h"

#define DLNAPROXY_VERSION "1.0"

#define SERVER_NAME "DLNAProxy"

#define CLIENT_CACHE_SLOTS 20

#define _(string) (string)

/* statup time */
extern time_t startup_time;

extern struct runtime_vars_s runtime_vars;
/* runtime boolean flags */
extern int runtime_flags;
#define INOTIFY_MASK          0x0001
#define TIVO_MASK             0x0002
#define DLNA_STRICT_MASK      0x0004

#define SETFLAG(mask)	runtime_flags |= mask
#define GETFLAG(mask)	runtime_flags & mask
#define CLEARFLAG(mask)	runtime_flags &= ~mask

extern const char * pidfilename;

extern char uuidvalue[];

#define MODELNAME_MAX_LEN (64)
extern char modelname[];

#define MODELNUMBER_MAX_LEN (16)
extern char modelnumber[];

#define SERIALNUMBER_MAX_LEN (16)
extern char serialnumber[];

#define PRESENTATIONURL_MAX_LEN (64)
extern char presentationurl[];

/* lan addresses */
/* MAX_LAN_ADDR : maximum number of interfaces
 * to listen to SSDP traffic */
#define MAX_LAN_ADDR (4)
extern int n_lan_addr;
extern struct lan_addr_s lan_addr[];

extern const char * minissdpdsocketpath;

/* UPnP-A/V [DLNA] */
#define FRIENDLYNAME_MAX_LEN (64)
extern char friendly_name[];
extern char log_path[];
extern struct client_cache_s clients[CLIENT_CACHE_SLOTS];
extern volatile short int quitting;
extern volatile uint32_t updateID;

#endif
