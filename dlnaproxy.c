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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <sys/param.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>

#include "config.h"

#include "upnpglobalvars.h"
#include "upnpdescgen.h"
#include "getifaddr.h"
#include "options.h"
#include "utils.h"
#include "minissdp.h"
#include "dlnaproxytypes.h"
#include "daemonize.h"
#include "upnpevents.h"
#include "tcptunnel.h"
#include "log.h"

/* Handler for the SIGTERM signal (kill) 
 * SIGINT is also handled */
static void
sigterm(int sig)
{
	/*int save_errno = errno;*/
	signal(sig, SIG_IGN);	/* Ignore this signal while we are quitting */

	DPRINTF(E_WARN, L_GENERAL, "received signal %d, good-bye\n", sig);

	quitting = 1;
	/*errno = save_errno;*/
}

/* record the startup time, for returning uptime */
static void
set_startup_time(void)
{
	startup_time = time(NULL);
}

/* parselanaddr()
 * parse address with mask
 * ex: 192.168.1.1/24
 * return value : 
 *    0 : ok
 *   -1 : error */
static int
parselanaddr(struct lan_addr_s * lan_addr, const char * str)
{
	const char * p;
	int nbits = 24;
	int n;
	p = str;
	while(*p && *p != '/' && !isspace(*p))
		p++;
	n = p - str;
	if(*p == '/')
	{
		nbits = atoi(++p);
		while(*p && !isspace(*p))
			p++;
	}
	if(n>15)
	{
		DPRINTF(E_OFF, L_GENERAL, "Error parsing address/mask: %s\n", str);
		return -1;
	}
	memcpy(lan_addr->str, str, n);
	lan_addr->str[n] = '\0';
	if(!inet_aton(lan_addr->str, &lan_addr->addr))
	{
		DPRINTF(E_OFF, L_GENERAL, "Error parsing address: %s\n", str);
		return -1;
	}
	lan_addr->mask.s_addr = htonl(nbits ? (0xffffffff << (32 - nbits)) : 0);
	return 0;
}

static void
getfriendlyname(char * buf, int len)
{
	char * dot = NULL;
	char * hn = calloc(1, 256);
	int off;

	if( gethostname(hn, 256) == 0 )
	{
		strncpy(buf, hn, len-1);
		buf[len] = '\0';
		dot = strchr(buf, '.');
		if( dot )
			*dot = '\0';
	}
	else
	{
		strcpy(buf, "Unknown");
	}
	free(hn);

	off = strlen(buf);
	off += snprintf(buf+off, len-off, ": ");
	char * logname;
	logname = getenv("LOGNAME");
#ifndef STATIC // Disable for static linking
	if( !logname )
	{
		struct passwd * pwent;
		pwent = getpwuid(getuid());
		if( pwent )
			logname = pwent->pw_name;
	}
#endif
	snprintf(buf+off, len-off, "%s", logname?logname:"Unknown");
}

/* init phase :
 * 1) read configuration file
 * 2) read command line arguments
 * 3) daemonize
 * 4) check and write pid file
 * 5) set startup time stamp
 * 6) set signal handlers */
static int
init(int argc, char * * argv)
{
	int i;
	int pid;
	int debug_flag = 0;
	int options_flag = 0;
	struct sigaction sa;
	/*const char * logfilename = 0;*/
	const char * presurl = 0;
	const char * optionsfile = "/etc/dlnaproxy.conf";
	char mac_str[13];
	char * string, * word;
	char * path;
	char real_path[PATH_MAX];
	char ip_addr[INET_ADDRSTRLEN + 3] = {'\0'};

	/* first check if "-f" option is used */
	for(i=2; i<argc; i++)
	{
		if(0 == strcmp(argv[i-1], "-f"))
		{
			optionsfile = argv[i];
			options_flag = 1;
			break;
		}
	}

	/* set up uuid based on mac address */
	if( getsyshwaddr(mac_str, sizeof(mac_str)) < 0 )
	{
		DPRINTF(E_OFF, L_GENERAL, "No MAC address found.  Falling back to generic UUID.\n");
		strcpy(mac_str, "554e4b4e4f57");
	}
	strcpy(uuidvalue+5, "4d696e69-444c-164e-9d41-");
	strncat(uuidvalue, mac_str, 12);

	getfriendlyname(friendly_name, FRIENDLYNAME_MAX_LEN);
	
	runtime_vars.port = -1;
	runtime_vars.notify_interval = 895;	/* seconds between SSDP announces */
	runtime_vars.rport = 50001;
	runtime_vars.rhost = "192.168.1.1";

	/* read options file first since
	 * command line arguments have final say */
	if(readoptionsfile(optionsfile) < 0)
	{
		/* only error if file exists or using -f */
		if(access(optionsfile, F_OK) == 0 || options_flag)
			fprintf(stderr, "Error reading configuration file %s\n", optionsfile);
	}
	else
	{
		for(i=0; i<num_options; i++)
		{
			switch(ary_options[i].id)
			{
			case UPNPIFNAME:
				for( string = ary_options[i].value; (word = strtok(string, ",")); string = NULL )
				{
					if(n_lan_addr < MAX_LAN_ADDR)
					{
						if(getifaddr(word, ip_addr, sizeof(ip_addr)) >= 0)
						{
							if( *ip_addr && parselanaddr(&lan_addr[n_lan_addr], ip_addr) == 0 )
								if(n_lan_addr < MAX_LAN_ADDR)
									n_lan_addr++;
						}
						else
							fprintf(stderr, "Interface %s not found, ignoring.\n", word);
					}
					else
					{
						fprintf(stderr, "Too many listening ips (max: %d), ignoring %s\n",
				    		    MAX_LAN_ADDR, word);
					}
				}
				break;
			case UPNPLISTENING_IP:
				if(n_lan_addr < MAX_LAN_ADDR)
				{
					if(parselanaddr(&lan_addr[n_lan_addr],
					             ary_options[i].value) == 0)
						n_lan_addr++;
				}
				else
				{
					fprintf(stderr, "Too many listening ips (max: %d), ignoring %s\n",
			    		    MAX_LAN_ADDR, ary_options[i].value);
				}
				break;
			case UPNPPORT:
				runtime_vars.port = atoi(ary_options[i].value);
				break;
			case UPNPNOTIFY_INTERVAL:
				runtime_vars.notify_interval = atoi(ary_options[i].value);
				break;
			case UPNPSERIAL:
				strncpy(serialnumber, ary_options[i].value, SERIALNUMBER_MAX_LEN);
				serialnumber[SERIALNUMBER_MAX_LEN-1] = '\0';
				break;				
			case UPNPMODEL_NAME:
				strncpy(modelname, ary_options[i].value, MODELNAME_MAX_LEN);
				modelname[MODELNAME_MAX_LEN-1] = '\0';
				break;
			case UPNPMODEL_NUMBER:
				strncpy(modelnumber, ary_options[i].value, MODELNUMBER_MAX_LEN);
				modelnumber[MODELNUMBER_MAX_LEN-1] = '\0';
				break;
			case UPNPLOGDIR:
				path = realpath(ary_options[i].value, real_path);
				if( !path )
					path = (ary_options[i].value);
				make_dir(path, S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
				if( access(path, F_OK) != 0 )
				{
					DPRINTF(E_FATAL, L_GENERAL, "Log path not accessible! [%s]\n", path);
					break;
				}
				strncpy(log_path, path, PATH_MAX);
				break;
			case UPNPMINISSDPDSOCKET:
				minissdpdsocketpath = ary_options[i].value;
				break;
			case UPNPREMOTEUUID:
				runtime_vars.ruuid = ary_options[i].value;
				strcpy(uuidvalue+5, runtime_vars.ruuid);
				break;
			case UPNPREMOTEPORT:
				runtime_vars.rport = atoi(ary_options[i].value);
				break;
			case UPNPREMOTEHOST:
				runtime_vars.rhost = ary_options[i].value;
				break;
			case UPNPDESCPATH:
				runtime_vars.path = ary_options[i].value;
				break;
			default:
				fprintf(stderr, "Unknown option with value %s in file %s\n",
				        ary_options[i].value, optionsfile);
			}
		}
	}
	if( log_path[0] == '\0' )
	{
		strncpy(log_path, DEFAULT_LOG_PATH, PATH_MAX);
	}

	/* command line arguments processing */
	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='-')
		{
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
		}
		else if(strcmp(argv[i], "--help")==0)
		{
			runtime_vars.port = 0;
			break;
		}
		else switch(argv[i][1])
		{
		case 't':
			if(i+1 < argc)
				runtime_vars.notify_interval = atoi(argv[++i]);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		/*case 'l':
			logfilename = argv[++i];
			break;*/
		case 'p':
			if(i+1 < argc)
				runtime_vars.port = atoi(argv[++i]);
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'P':
			if(i+1 < argc)
				pidfilename = argv[++i];
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'd':
			debug_flag = 1;
			break;
		case 'a':
			if(i+1 < argc)
			{
				int address_already_there = 0;
				int j;
				i++;
				for(j=0; j<n_lan_addr; j++)
				{
					struct lan_addr_s tmpaddr;
					parselanaddr(&tmpaddr, argv[i]);
					if(0 == strcmp(lan_addr[j].str, tmpaddr.str))
						address_already_there = 1;
				}
				if(address_already_there)
					break;
				if(n_lan_addr < MAX_LAN_ADDR)
				{
					if(parselanaddr(&lan_addr[n_lan_addr], argv[i]) == 0)
						n_lan_addr++;
				}
				else
				{
					fprintf(stderr, "Too many listening ips (max: %d), ignoring %s\n",
				    	    MAX_LAN_ADDR, argv[i]);
				}
			}
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'i':
			if(i+1 < argc)
			{
				int address_already_there = 0;
				int j;
				i++;
				if( getifaddr(argv[i], ip_addr, sizeof(ip_addr)) < 0 )
				{
					fprintf(stderr, "Network interface '%s' not found.\n",
						argv[i]);
					exit(-1);
				}
				for(j=0; j<n_lan_addr; j++)
				{
					struct lan_addr_s tmpaddr;
					parselanaddr(&tmpaddr, ip_addr);
					if(0 == strcmp(lan_addr[j].str, tmpaddr.str))
						address_already_there = 1;
				}
				if(address_already_there)
					break;
				if(n_lan_addr < MAX_LAN_ADDR)
				{
					if(parselanaddr(&lan_addr[n_lan_addr], ip_addr) == 0)
						n_lan_addr++;
				}
				else
				{
					fprintf(stderr, "Too many listening ips (max: %d), ignoring %s\n",
				    	    MAX_LAN_ADDR, argv[i]);
				}
			}
			else
				fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'f':
			i++;	/* discarding, the config file is already read */
			break;
		case 'h':
			runtime_vars.port = 0; // triggers help display
			break;
		case 'V':
			printf("Version " DLNAPROXY_VERSION "\n");
			exit(0);
			break;
		default:
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
		}
	}
	/* If no IP was specified, try to detect one */
	if( n_lan_addr < 1 )
	{
		if( (getsysaddr(ip_addr, sizeof(ip_addr)) < 0) &&
		    (getifaddr("eth0", ip_addr, sizeof(ip_addr)) < 0) &&
		    (getifaddr("eth1", ip_addr, sizeof(ip_addr)) < 0) )
		{
			DPRINTF(E_OFF, L_GENERAL, "No IP address automatically detected!\n");
		}
		if( *ip_addr && parselanaddr(&lan_addr[n_lan_addr], ip_addr) == 0 )
		{
			n_lan_addr++;
		}
	}

	if( (n_lan_addr==0) || (runtime_vars.port<=0) )
	{
		fprintf(stderr, "Usage:\n\t"
		        "%s [-d] [-f config_file]\n"
			"\t\t[-a listening_ip] [-p port]\n"
			/*"[-l logfile] " not functionnal */
			"\t\t[-t notify_interval] [-P pid_filename]\n"
			"\t\t[-V] [-h]\n"
		        "\nNotes:\n\tNotify interval is in seconds. Default is 895 seconds.\n"
			"\tDefault pid file is %s.\n"
			"\tWith -d dlnaproxy will run in debug mode (not daemonize).\n"
			"\t-h displays this text\n"
			"\t-V print the version number\n",
		        argv[0], pidfilename);
		return 1;
	}

	if(debug_flag)
	{
		pid = getpid();
		log_init(NULL, "general,inotify,http,ssdp=debug");
	}
	else
	{
#ifdef USE_DAEMON
		if(daemon(0, 0)<0) {
			perror("daemon()");
		}
		pid = getpid();
#else
		pid = daemonize();
#endif
		if( access(log_path, F_OK) != 0 )
			make_dir(log_path, S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
		sprintf(real_path, "%s/dlnaproxy.log", log_path);
		log_init(real_path, "general,inotify,http,ssdp=warn");
	}

	if(checkforrunning(pidfilename) < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "DLNAProxy is already running. EXITING.\n");
		return 1;
	}	

	set_startup_time();

	/* presentation url */
	if(presurl)
	{
		strncpy(presentationurl, presurl, PRESENTATIONURL_MAX_LEN);
		presentationurl[PRESENTATIONURL_MAX_LEN-1] = '\0';
	}
	else
	{
		snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
		         "http://%s:%d/", lan_addr[0].str, runtime_vars.port);
	}

	/* set signal handler */
	signal(SIGCLD, SIG_IGN);
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sigterm;
	if (sigaction(SIGTERM, &sa, NULL))
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set SIGTERM handler. EXITING.\n");
	}
	if (sigaction(SIGINT, &sa, NULL))
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set SIGINT handler. EXITING.\n");
	}

	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		DPRINTF(E_FATAL, L_GENERAL, "Failed to ignore SIGPIPE signals. EXITING.\n");
	}

	writepidfile(pidfilename, pid);

	return 0;
}

/* === main === */
/* process HTTP or SSDP requests */
int
main(int argc, char * * argv)
{
	int i;
	int sudp = -1, shttpl = -1;
	int snotify[MAX_LAN_ADDR];
	fd_set readset;	/* for select() */
	fd_set writeset;
	struct timeval timeout, timeofday, lastnotifytime = {0, 0};
	int max_fd = -1;
	pid_t tcplistener_pid = 0;

	if(init(argc, argv) != 0)
		return 1;

	DPRINTF(E_WARN, L_GENERAL, "Starting " SERVER_NAME " version " DLNAPROXY_VERSION ".\n");

	sudp = OpenAndConfSSDPReceiveSocket(n_lan_addr, lan_addr);
	if(sudp < 0)
	{
		DPRINTF(E_INFO, L_GENERAL, "Failed to open socket for receiving SSDP. Trying to use MiniSSDPd\n");
		if(SubmitServicesToMiniSSDPD(lan_addr[0].str, runtime_vars.port) < 0) {
			DPRINTF(E_FATAL, L_GENERAL, "Failed to connect to MiniSSDPd. EXITING");
			return 1;
		}
	}

	/* open socket for sending notifications */
	if(OpenAndConfSSDPNotifySockets(snotify) < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to open sockets for sending SSDP notify "
	                "messages. EXITING\n");
	}

	SendSSDPGoodbye(snotify, n_lan_addr);

	/* create tcp tunnel for Http forwarding*/
	tcplistener_pid = fork();
	if( tcplistener_pid == 0) // child (listener) process
	{
		if(build_server() == 0)
		{
			while(!quitting)
			{
				/* process tunnel packets */
				if (wait_for_clients() == 0)
				{
					if (build_tunnel() == 0)
					{
						use_tunnel();
					}
				}
			}
		}
		close_server();
		DPRINTF(E_INFO, L_GENERAL, "Shutting down tcp tunnel thread\n");
		exit(EXIT_SUCCESS);
	}else if( tcplistener_pid < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Failed to create tcp tunnel thread\n");
		exit(EXIT_SUCCESS);
	}else{
		/* main loop */
		while(!quitting)
		{
			/* Check if we need to send SSDP NOTIFY messages and do it if
			 * needed */
			if(gettimeofday(&timeofday, 0) < 0)
			{
				DPRINTF(E_ERROR, L_GENERAL, "gettimeofday(): %s\n", strerror(errno));
				timeout.tv_sec = runtime_vars.notify_interval;
				timeout.tv_usec = 0;
			}
			else
			{
				/* the comparaison is not very precise but who cares ? */
				if(timeofday.tv_sec >= (lastnotifytime.tv_sec + runtime_vars.notify_interval))
				{
					//SendSSDPNotifies3(snotify, "store", (unsigned short)runtime_vars.port, "/desc/device.xml", (runtime_vars.notify_interval << 1)+10);
					SendSSDPNotifies2(snotify, (unsigned short)runtime_vars.port, runtime_vars.path, (runtime_vars.notify_interval << 1)+10);
					memcpy(&lastnotifytime, &timeofday, sizeof(struct timeval));
					timeout.tv_sec = runtime_vars.notify_interval;
					timeout.tv_usec = 0;
				}
				else
				{
					timeout.tv_sec = lastnotifytime.tv_sec + runtime_vars.notify_interval
				    	             - timeofday.tv_sec;
					if(timeofday.tv_usec > lastnotifytime.tv_usec)
					{
						timeout.tv_usec = 1000000 + lastnotifytime.tv_usec
					    	              - timeofday.tv_usec;
						timeout.tv_sec--;
					}
					else
					{
						timeout.tv_usec = lastnotifytime.tv_usec - timeofday.tv_usec;
					}
				}
			}

			/* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
			FD_ZERO(&readset);

			if (sudp >= 0) 
			{
				FD_SET(sudp, &readset);
				max_fd = MAX(max_fd, sudp);
			}
		
			FD_ZERO(&writeset);
			upnpevents_selectfds(&readset, &writeset, &max_fd);

			if(select(max_fd+1, &readset, &writeset, 0, &timeout) < 0)
			{
				if(quitting) goto shutdown;
				DPRINTF(E_ERROR, L_GENERAL, "select(all): %s\n", strerror(errno));
				DPRINTF(E_FATAL, L_GENERAL, "Failed to select open sockets. EXITING\n");
			}
			upnpevents_processfds(&readset, &writeset);
		
			/* process SSDP packets */
			if(sudp >= 0 && FD_ISSET(sudp, &readset))
			{
				/*DPRINTF(E_DEBUG, L_GENERAL, "Received UDP Packet\n");*/
				//ProcessSSDPRequest(sudp, "store", (unsigned short)runtime_vars.port, "/desc/device.xml");
				ProcessSSDPRequest(sudp, (unsigned short)runtime_vars.port, runtime_vars.path);
			}
		}
	}

shutdown:
	/* kill the listener */
	DPRINTF(E_DEBUG, L_GENERAL, "Trying to kill tunnel thread: %d\n", tcplistener_pid);
	if( tcplistener_pid )
	{
		kill(tcplistener_pid, 9);
	}
	/* close out open sockets */
	if (sudp >= 0) close(sudp);
	if (shttpl >= 0) close(shttpl);
	
	if(SendSSDPGoodbye(snotify, n_lan_addr) < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Failed to broadcast good-bye notifications\n");
	}
	for(i=0; i<n_lan_addr; i++)
		close(snotify[i]);

	if(unlink(pidfilename) < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Failed to remove pidfile %s: %s\n", pidfilename, strerror(errno));
	}

	freeoptions();

	exit(EXIT_SUCCESS);
}
