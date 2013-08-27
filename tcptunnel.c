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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include <time.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "tcptunnel.h"
#include "upnpglobalvars.h"
#include "log.h"

struct struct_rc rc;

void close_server(void)
{
	close(rc.server_socket);
}

int build_server(void)
{
	memset(&rc.server_addr, 0, sizeof(rc.server_addr));

	rc.server_addr.sin_port = htons(runtime_vars.port);
	rc.server_addr.sin_family = AF_INET;
	rc.server_addr.sin_addr.s_addr = INADDR_ANY;

	rc.server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (rc.server_socket < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to open socket for tcp tunnel. EXITING.\n");
		return 1;
	}

	//rc.server_addr.sin_addr.s_addr = inet_addr(/*bindadr*/);

	if (bind(rc.server_socket, (struct sockaddr *) &rc.server_addr, sizeof(rc.server_addr)) < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to bind to socket for tcp tunnel. EXITING.\n");
		return 1;
	}

	if (listen(rc.server_socket, 1) < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to listen on socket for tcp tunnel. EXITING.\n");
		return 1;
	}

	return 0;
}

int wait_for_clients(void)
{
	int client_addr_size = sizeof(struct sockaddr_in);

	rc.client_socket = accept(rc.server_socket, (struct sockaddr *) &rc.client_addr, (socklen_t *) &client_addr_size);
	if (rc.client_socket < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to wait for connections on socket for tcp tunnel. EXITING.\n");
		return 1;
	}

	//printf("> %s tcptunnel: request from %s\n", get_current_timestamp(), inet_ntoa(rc.client_addr.sin_addr));

	return 0;
}

int build_tunnel(void)
{
	rc.remote_host = gethostbyname(runtime_vars.rhost);
	if (rc.remote_host == NULL)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to resolve hostname for tcp tunnel. EXITING.\n");
		return 1;
	}

	memset(&rc.remote_addr, 0, sizeof(rc.remote_addr));

	rc.remote_addr.sin_family = AF_INET;
	rc.remote_addr.sin_port = htons(runtime_vars.rport);

	memcpy(&rc.remote_addr.sin_addr.s_addr, rc.remote_host->h_addr, rc.remote_host->h_length);

	rc.remote_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (rc.remote_socket < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to open socket for tcp tunnel. EXITING.\n");
		return 1;
	}

	if (connect(rc.remote_socket, (struct sockaddr *) &rc.remote_addr, sizeof(rc.remote_addr)) < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to connect to remote host for tcp tunnel. EXITING.\n");
		return 1;
	}

	return 0;
}

int use_tunnel(void)
{
	fd_set io;
	char buffer[SIZE];

	while(!quitting)
	{
		FD_ZERO(&io);
		FD_SET(rc.client_socket, &io);
		FD_SET(rc.remote_socket, &io);

		memset(buffer, 0, SIZE);

		if (select(fd(), &io, NULL, NULL, NULL) < 0 )
		{
			DPRINTF(E_FATAL, L_GENERAL, "Failed to use selected tcp tunnel. EXITING.\n");
			break;
		}

		if (FD_ISSET(rc.client_socket, &io))
		{
			int count = recv(rc.client_socket, buffer, SIZE, 0);
			if (count < 0)
			{
				DPRINTF(E_FATAL, L_GENERAL, "Failed to receive on client socket for tcp tunnel. EXITING.\n");
				close(rc.client_socket);
				close(rc.remote_socket);
				return 1;
			}

			if (count == 0)
			{
				close(rc.client_socket);
				close(rc.remote_socket);
				return 0;
			}

			send(rc.remote_socket, buffer, count, 0);

			//printf("> %s > ", get_current_timestamp());
			//fwrite(buffer, sizeof(char), count, stdout);
			//fflush(stdout);
		}

		if (FD_ISSET(rc.remote_socket, &io))
		{
			int count = recv(rc.remote_socket, buffer, SIZE, 0);
			if (count < 0)
			{
				DPRINTF(E_FATAL, L_GENERAL, "Failed to receive on remote socket for tcp tunnel. EXITING.\n");
				close(rc.client_socket);
				close(rc.remote_socket);
				return 1;
			}

			if (count == 0)
			{
				close(rc.client_socket);
				close(rc.remote_socket);
				return 0;
			}

			send(rc.client_socket, buffer, count, 0);

			//fwrite(buffer, sizeof(char), count, stdout);
			//fflush(stdout);
		}
	}

	return 0;
}

int fd(void)
{
	unsigned int fd = rc.client_socket;
	if (fd < rc.remote_socket)
	{
		fd = rc.remote_socket;
	}
	return fd + 1;
}

char *get_current_timestamp(void)
{
	static char date_str[SIZE];
	time_t date;

	time(&date);
	strftime(date_str, SIZE - 1, "%a %d %H:%M:%S", localtime(&date));

	return date_str;
}
