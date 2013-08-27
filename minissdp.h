/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
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
#ifndef __MINISSDP_H__
#define __MINISSDP_H__

#define DLNAPROXY_SERVER_STRING OS_VERSION " DLNADOC/1.50 UPnP/1.0 DLNAProxy/1.0"

#define FLAG_TIMEOUT            0x00000001
#define FLAG_SID                0x00000002
#define FLAG_RANGE              0x00000004
#define FLAG_HOST               0x00000008

#define FLAG_HTML               0x00000080
#define FLAG_INVALID_REQ        0x00000010

#define FLAG_CHUNKED            0x00000100
#define FLAG_TIMESEEK           0x00000200
#define FLAG_REALTIMEINFO       0x00000400
#define FLAG_PLAYSPEED          0x00000800
#define FLAG_XFERSTREAMING      0x00001000
#define FLAG_XFERINTERACTIVE    0x00002000
#define FLAG_XFERBACKGROUND     0x00004000
#define FLAG_CAPTION            0x00008000

#define FLAG_DLNA               0x00100000
#define FLAG_MIME_AVI_DIVX      0x00200000
#define FLAG_MIME_AVI_AVI       0x00400000
#define FLAG_MIME_FLAC_FLAC     0x00800000
#define FLAG_MIME_WAV_WAV       0x01000000
#define FLAG_NO_RESIZE          0x02000000
#define FLAG_MS_PFS             0x04000000 // Microsoft PlaysForSure client
#define FLAG_SAMSUNG            0x08000000
#define FLAG_AUDIO_ONLY         0x10000000

#define FLAG_FREE_OBJECT_ID     0x00000001
#define FLAG_ROOT_CONTAINER     0x00000002

int
OpenAndConfSSDPReceiveSocket();
/* OpenAndConfSSDPReceiveSocket(int n_lan_addr, struct lan_addr_s * lan_addr);*/

/*int
OpenAndConfSSDPNotifySocket(const char * addr);*/

int
OpenAndConfSSDPNotifySockets(int * sockets);
/*OpenAndConfSSDPNotifySockets(int * sockets,
                             struct lan_addr_s * lan_addr, int n_lan_addr);*/

/*void
SendSSDPNotifies(int s, const char * host, unsigned short port,
                 unsigned int lifetime);*/
void
SendSSDPNotifies2(int * sockets,
                  unsigned short port,
                  const char * path,
                  unsigned int lifetime);
void
SendSSDPNotifies3(int * sockets,
                  const char * host,
                  unsigned short port,
                  const char * path,
                  unsigned int lifetime);
/*SendSSDPNotifies2(int * sockets, struct lan_addr_s * lan_addr, int n_lan_addr,
                  unsigned short port,
                  unsigned int lifetime);*/

void
ProcessSSDPRequest(int s, unsigned short port, const char * path);
/*ProcessSSDPRequest(int s, struct lan_addr_s * lan_addr, int n_lan_addr,
                   unsigned short port);*/

int
SendSSDPGoodbye(int * sockets, int n);

int
SubmitServicesToMiniSSDPD(const char * host, unsigned short port);

#endif

