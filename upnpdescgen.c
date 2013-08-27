/* $Id: upnpdescgen.c,v 1.18 2011/05/02 23:50:52 jmaggard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 *
 * Copyright (c) 2006-2008, Thomas Bernard
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
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "getifaddr.h"
#include "upnpdescgen.h"
#include "upnpglobalvars.h"
#include "upnpdescstrings.h"

#undef DESC_DEBUG

static const char * const upnptypes[] =
{
	"string",
	"boolean",
	"ui2",
	"ui4",
	"i4",
	"uri",
	"int",
	"bin.base64"
};

static const char * const upnpdefaultvalues[] =
{
	0,
	"Unconfigured"
};

static const char * const upnpallowedvalues[] =
{
	0,			/* 0 */
	"DSL",			/* 1 */
	"POTS",
	"Cable",
	"Ethernet",
	0,
	"Up",			/* 6 */
	"Down",
	"Initializing",
	"Unavailable",
	0,
	"TCP",			/* 11 */
	"UDP",
	0,
	"Unconfigured",		/* 14 */
	"IP_Routed",
	"IP_Bridged",
	0,
	"Unconfigured",		/* 18 */
	"Connecting",
	"Connected",
	"PendingDisconnect",
	"Disconnecting",
	"Disconnected",
	0,
	"ERROR_NONE",		/* 25 */
	0,
	"OK",			/* 27 */
	"ContentFormatMismatch",
	"InsufficientBandwidth",
	"UnreliableChannel",
	"Unknown",
	0,
	"Input",		/* 33 */
	"Output",
	0,
	"BrowseMetadata",	/* 36 */
	"BrowseDirectChildren",
	0,
	"COMPLETED",		/* 39 */
	"ERROR",
	"IN_PROGRESS",
	"STOPPED",
	0,
	0,		/* 44 */
	0,
	"0",			/* 46 */
	0,
	"",			/* 48 */
	0
};

static const char xmlver[] = 
	"<?xml version=\"1.0\"?>\r\n";
static const char root_service[] =
	"scpd xmlns=\"urn:schemas-upnp-org:service-1-0\"";
static const char root_device[] = 
	"root xmlns=\"urn:schemas-upnp-org:device-1-0\"";

static const struct argument AddPortMappingArgs[] =
{
	{NULL, 1, 11},
	{NULL, 1, 12},
	{NULL, 1, 14},
	{NULL, 1, 13},
	{NULL, 1, 15},
	{NULL, 1, 9},
	{NULL, 1, 16},
	{NULL, 1, 10},
	{NULL, 0, 0}
};

static const struct argument DeletePortMappingArgs[] = 
{
	{NULL, 1, 11},
	{NULL, 1, 12},
	{NULL, 1, 14},
	{NULL, 0, 0}
};

static const struct argument SetConnectionTypeArgs[] =
{
	{NULL, 1, 0},
	{NULL, 0, 0}
};

static const struct argument GetConnectionTypeInfoArgs[] =
{
	{NULL, 2, 0},
	{NULL, 2, 1},
	{NULL, 0, 0}
};

static const struct argument GetNATRSIPStatusArgs[] =
{
	{NULL, 2, 5},
	{NULL, 2, 6},
	{NULL, 0, 0}
};

static const struct argument GetGenericPortMappingEntryArgs[] =
{
	{NULL, 1, 8},
	{NULL, 2, 11},
	{NULL, 2, 12},
	{NULL, 2, 14},
	{NULL, 2, 13},
	{NULL, 2, 15},
	{NULL, 2, 9},
	{NULL, 2, 16},
	{NULL, 2, 10},
	{NULL, 0, 0}
};

static const struct argument GetSpecificPortMappingEntryArgs[] =
{
	{NULL, 1, 11},
	{NULL, 1, 12},
	{NULL, 1, 14},
	{NULL, 2, 13},
	{NULL, 2, 15},
	{NULL, 2, 9},
	{NULL, 2, 16},
	{NULL, 2, 10},
	{NULL, 0, 0}
};

/* For ConnectionManager */
static const struct argument GetProtocolInfoArgs[] =
{
	{"Source", 2, 0},
	{"Sink", 2, 1},
	{NULL, 0, 0}
};

static const struct argument PrepareForConnectionArgs[] =
{
	{"RemoteProtocolInfo", 1, 6},
	{"PeerConnectionManager", 1, 4},
	{"PeerConnectionID", 1, 7},
	{"Direction", 1, 5},
	{"ConnectionID", 2, 7},
	{"AVTransportID", 2, 8},
	{"RcsID", 2, 9},
	{NULL, 0, 0}
};

static const struct argument ConnectionCompleteArgs[] =
{
	{"ConnectionID", 1, 7},
	{NULL, 0, 0}
};

static const struct argument GetCurrentConnectionIDsArgs[] =
{
	{"ConnectionIDs", 2, 2},
	{NULL, 0, 0}
};

static const struct argument GetCurrentConnectionInfoArgs[] =
{
	{"ConnectionID", 1, 7},
	{"RcsID", 2, 9},
	{"AVTransportID", 2, 8},
	{"ProtocolInfo", 2, 6},
	{"PeerConnectionManager", 2, 4},
	{"PeerConnectionID", 2, 7},
	{"Direction", 2, 5},
	{"Status", 2, 3},
	{NULL, 0, 0}
};

static const struct action ConnectionManagerActions[] =
{
	{"GetProtocolInfo", GetProtocolInfoArgs}, /* R */
	//OPTIONAL {"PrepareForConnection", PrepareForConnectionArgs}, /* R */
	//OPTIONAL {"ConnectionComplete", ConnectionCompleteArgs}, /* R */
	{"GetCurrentConnectionIDs", GetCurrentConnectionIDsArgs}, /* R */
	{"GetCurrentConnectionInfo", GetCurrentConnectionInfoArgs}, /* R */
	{0, 0}
};

static const struct stateVar ConnectionManagerVars[] =
{
	{"SourceProtocolInfo", 1<<7, 0, 0, 44}, /* required */
	{"SinkProtocolInfo", 1<<7, 0, 0, 48}, /* required */
	{"CurrentConnectionIDs", 1<<7, 0, 0, 46}, /* required */
	{"A_ARG_TYPE_ConnectionStatus", 0, 0, 27}, /* required */
	{"A_ARG_TYPE_ConnectionManager", 0, 0}, /* required */
	{"A_ARG_TYPE_Direction", 0, 0, 33}, /* required */
	{"A_ARG_TYPE_ProtocolInfo", 0, 0}, /* required */
	{"A_ARG_TYPE_ConnectionID", 4, 0}, /* required */
	{"A_ARG_TYPE_AVTransportID", 4, 0}, /* required */
	{"A_ARG_TYPE_RcsID", 4, 0}, /* required */
	{0, 0}
};

static const struct argument GetSearchCapabilitiesArgs[] =
{
	{"SearchCaps", 2, 10},
	{0, 0}
};

static const struct argument GetSortCapabilitiesArgs[] =
{
	{"SortCaps", 2, 11},
	{0, 0}
};

static const struct argument GetSystemUpdateIDArgs[] =
{
	{"Id", 2, 12},
	{0, 0}
};

static const struct argument BrowseArgs[] =
{
	{"ObjectID", 1, 1},
	{"BrowseFlag", 1, 4},
	{"Filter", 1, 5},
	{"StartingIndex", 1, 7},
	{"RequestedCount", 1, 8},
	{"SortCriteria", 1, 6},
	{"Result", 2, 2},
	{"NumberReturned", 2, 8},
	{"TotalMatches", 2, 8},
	{"UpdateID", 2, 9},
	{0, 0}
};

static const struct argument SearchArgs[] =
{
	{"ContainerID", 1, 1},
	{"SearchCriteria", 1, 3},
	{"Filter", 1, 5},
	{"StartingIndex", 1, 7},
	{"RequestedCount", 1, 8},
	{"SortCriteria", 1, 6},
	{"Result", 2, 2},
	{"NumberReturned", 2, 8},
	{"TotalMatches", 2, 8},
	{"UpdateID", 2, 9},
	{0, 0}
};

static const struct action ContentDirectoryActions[] =
{
	{"GetSearchCapabilities", GetSearchCapabilitiesArgs}, /* R */
	{"GetSortCapabilities", GetSortCapabilitiesArgs}, /* R */
	{"GetSystemUpdateID", GetSystemUpdateIDArgs}, /* R */
	{"Browse", BrowseArgs}, /* R */
	{"Search", SearchArgs}, /* O */
#if 0 // Not implementing optional features yet...
	{"CreateObject", CreateObjectArgs}, /* O */
	{"DestroyObject", DestroyObjectArgs}, /* O */
	{"UpdateObject", UpdateObjectArgs}, /* O */
	{"ImportResource", ImportResourceArgs}, /* O */
	{"ExportResource", ExportResourceArgs}, /* O */
	{"StopTransferResource", StopTransferResourceArgs}, /* O */
	{"GetTransferProgress", GetTransferProgressArgs}, /* O */
	{"DeleteResource", DeleteResourceArgs}, /* O */
	{"CreateReference", CreateReferenceArgs}, /* O */
#endif
	{0, 0}
};

static const struct stateVar ContentDirectoryVars[] =
{
	{"TransferIDs", 1<<7, 0, 0, 48}, /* 0 */
	{"A_ARG_TYPE_ObjectID", 0, 0},
	{"A_ARG_TYPE_Result", 0, 0},
	{"A_ARG_TYPE_SearchCriteria", 0, 0},
	{"A_ARG_TYPE_BrowseFlag", 0, 0, 36},
	/* Allowed Values : BrowseMetadata / BrowseDirectChildren */
	{"A_ARG_TYPE_Filter", 0, 0}, /* 5 */
	{"A_ARG_TYPE_SortCriteria", 0, 0},
	{"A_ARG_TYPE_Index", 3, 0},
	{"A_ARG_TYPE_Count", 3, 0},
	{"A_ARG_TYPE_UpdateID", 3, 0},
	//JM{"A_ARG_TYPE_TransferID", 3, 0}, /* 10 */
	//JM{"A_ARG_TYPE_TransferStatus", 0, 0, 39},
	/* Allowed Values : COMPLETED / ERROR / IN_PROGRESS / STOPPED */
	//JM{"A_ARG_TYPE_TransferLength", 0, 0},
	//JM{"A_ARG_TYPE_TransferTotal", 0, 0},
	//JM{"A_ARG_TYPE_TagValueList", 0, 0},
	//JM{"A_ARG_TYPE_URI", 5, 0}, /* 15 */
	{"SearchCapabilities", 0, 0},
	{"SortCapabilities", 0, 0},
	{"SystemUpdateID", 3|0x80, 0, 0, 255},
	//{"ContainerUpdateIDs", 0, 0},
	{0, 0}
};

static const struct argument GetIsAuthorizedArgs[] =
{
	{"DeviceID", 1, 0},
	{"Result", 2, 3},
	{NULL, 0, 0}
};

static const struct argument GetIsValidatedArgs[] =
{
	{"DeviceID", 1, 0},
	{"Result", 2, 3},
	{NULL, 0, 0}
};

static const struct argument GetRegisterDeviceArgs[] =
{
	{"RegistrationReqMsg", 1, 1},
	{"RegistrationRespMsg", 2, 2},
	{NULL, 0, 0}
};

static const struct action X_MS_MediaReceiverRegistrarActions[] =
{
	{"IsAuthorized", GetIsAuthorizedArgs}, /* R */
	{"IsValidated", GetIsValidatedArgs}, /* R */
#if 0 // Not needed?  WMP12 still works.  Need to check with 360 and WMP11.
	{"RegisterDevice", GetRegisterDeviceArgs}, /* R */
#endif
	{0, 0}
};

static const struct stateVar X_MS_MediaReceiverRegistrarVars[] =
{
	{"A_ARG_TYPE_DeviceID", 0, 0},
	{"A_ARG_TYPE_RegistrationReqMsg", 7, 0},
	{"A_ARG_TYPE_RegistrationRespMsg", 7, 0},
	{"A_ARG_TYPE_Result", 6, 0},
	{"AuthorizationDeniedUpdateID", 3, 0},
	{"AuthorizationGrantedUpdateID", 3, 0},
	{"ValidationRevokedUpdateID", 3, 0},
	{"ValidationSucceededUpdateID", 3, 0},
	{0, 0}
};

/* WANCfg.xml */
/* See UPnP_IGD_WANCommonInterfaceConfig 1.0.pdf */

static const struct argument GetCommonLinkPropertiesArgs[] =
{
	{NULL, 2, 0},
	{NULL, 2, 1},
	{NULL, 2, 2},
	{NULL, 2, 3},
	{NULL, 0, 0}
};

static const struct argument GetTotalBytesSentArgs[] =
{
	{NULL, 2, 4},
	{NULL, 0, 0}
};

static const struct argument GetTotalBytesReceivedArgs[] =
{
	{NULL, 2, 5},
	{NULL, 0, 0}
};

static const struct argument GetTotalPacketsSentArgs[] =
{
	{NULL, 2, 6},
	{NULL, 0, 0}
};

static const struct argument GetTotalPacketsReceivedArgs[] =
{
	{NULL, 2, 7},
	{NULL, 0, 0}
};

static const struct serviceDesc scpdContentDirectory =
{ ContentDirectoryActions, ContentDirectoryVars };

static const struct serviceDesc scpdConnectionManager =
{ ConnectionManagerActions, ConnectionManagerVars };

static const struct serviceDesc scpdX_MS_MediaReceiverRegistrar =
{ X_MS_MediaReceiverRegistrarActions, X_MS_MediaReceiverRegistrarVars };

/* strcat_str()
 * concatenate the string and use realloc to increase the
 * memory buffer if needed. */
static char *
strcat_str(char * str, int * len, int * tmplen, const char * s2)
{
	char *p;
	int s2len;
	s2len = (int)strlen(s2);
	if(*tmplen <= (*len + s2len))
	{
		if(s2len < 256)
			*tmplen += 256;
		else
			*tmplen += s2len + 1;
		p = realloc(str, *tmplen);
		if (!p)
		{
			if(s2len < 256)
				*tmplen -= 256;
			else
				*tmplen -= s2len + 1;
			return str;
		}
		else
			str = p;
	}
	/*strcpy(str + *len, s2); */
	memcpy(str + *len, s2, s2len + 1);
	*len += s2len;
	return str;
}

/* strcat_char() :
 * concatenate a character and use realloc to increase the
 * size of the memory buffer if needed */
static char *
strcat_char(char * str, int * len, int * tmplen, char c)
{
	char *p;
	if(*tmplen <= (*len + 1))
	{
		*tmplen += 256;
		p = (char *)realloc(str, *tmplen);
		if (!p)
		{
			*tmplen -= 256;
			return str;
		}
		else
			str = p;
	}
	str[*len] = c;
	(*len)++;
	return str;
}

/* genServiceDesc() :
 * Generate service description with allowed methods and 
 * related variables. */
static char *
genServiceDesc(int * len, const struct serviceDesc * s)
{
	int i, j;
	const struct action * acts;
	const struct stateVar * vars;
	const struct argument * args;
	const char * p;
	char * str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	/*strcpy(str, xmlver); */
	*len = strlen(xmlver);
	memcpy(str, xmlver, *len + 1);
	
	acts = s->actionList;
	vars = s->serviceStateTable;

	str = strcat_char(str, len, &tmplen, '<');
	str = strcat_str(str, len, &tmplen, root_service);
	str = strcat_char(str, len, &tmplen, '>');

	str = strcat_str(str, len, &tmplen,
		"<specVersion><major>1</major><minor>0</minor></specVersion>");

	i = 0;
	str = strcat_str(str, len, &tmplen, "<actionList>");
	while(acts[i].name)
	{
		str = strcat_str(str, len, &tmplen, "<action><name>");
		str = strcat_str(str, len, &tmplen, acts[i].name);
		str = strcat_str(str, len, &tmplen, "</name>");
		/* argument List */
		args = acts[i].args;
		if(args)
		{
			str = strcat_str(str, len, &tmplen, "<argumentList>");
			j = 0;
			while(args[j].dir)
			{
				str = strcat_str(str, len, &tmplen, "<argument><name>");
				p = vars[args[j].relatedVar].name;
				str = strcat_str(str, len, &tmplen, (args[j].name ? args[j].name : p));
				str = strcat_str(str, len, &tmplen, "</name><direction>");
				str = strcat_str(str, len, &tmplen, args[j].dir==1?"in":"out");
				str = strcat_str(str, len, &tmplen,
						"</direction><relatedStateVariable>");
				str = strcat_str(str, len, &tmplen, p);
				str = strcat_str(str, len, &tmplen,
						"</relatedStateVariable></argument>");
				j++;
			}
			str = strcat_str(str, len, &tmplen,"</argumentList>");
		}
		str = strcat_str(str, len, &tmplen, "</action>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</actionList><serviceStateTable>");
	i = 0;
	while(vars[i].name)
	{
		str = strcat_str(str, len, &tmplen,
				"<stateVariable sendEvents=\"");
		str = strcat_str(str, len, &tmplen, (vars[i].itype & 0x80)?"yes":"no");
		str = strcat_str(str, len, &tmplen, "\"><name>");
		str = strcat_str(str, len, &tmplen, vars[i].name);
		str = strcat_str(str, len, &tmplen, "</name><dataType>");
		str = strcat_str(str, len, &tmplen, upnptypes[vars[i].itype & 0x0f]);
		str = strcat_str(str, len, &tmplen, "</dataType>");
		if(vars[i].iallowedlist)
		{
		  str = strcat_str(str, len, &tmplen, "<allowedValueList>");
		  for(j=vars[i].iallowedlist; upnpallowedvalues[j]; j++)
		  {
		    str = strcat_str(str, len, &tmplen, "<allowedValue>");
		    str = strcat_str(str, len, &tmplen, upnpallowedvalues[j]);
		    str = strcat_str(str, len, &tmplen, "</allowedValue>");
		  }
		  str = strcat_str(str, len, &tmplen, "</allowedValueList>");
		}
		/*if(vars[i].defaultValue) */
		if(vars[i].idefault)
		{
		  str = strcat_str(str, len, &tmplen, "<defaultValue>");
		  /*str = strcat_str(str, len, &tmplen, vars[i].defaultValue); */
		  str = strcat_str(str, len, &tmplen, upnpdefaultvalues[vars[i].idefault]);
		  str = strcat_str(str, len, &tmplen, "</defaultValue>");
		}
		str = strcat_str(str, len, &tmplen, "</stateVariable>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</serviceStateTable></scpd>");
	str[*len] = '\0';
	return str;
}

/* genContentDirectory() :
 * Generate the ContentDirectory xml description */
char *
genContentDirectory(int * len)
{
	return genServiceDesc(len, &scpdContentDirectory);
}

/* genConnectionManager() :
 * Generate the ConnectionManager xml description */
char *
genConnectionManager(int * len)
{
	return genServiceDesc(len, &scpdConnectionManager);
}

/* genX_MS_MediaReceiverRegistrar() :
 * Generate the X_MS_MediaReceiverRegistrar xml description */
char *
genX_MS_MediaReceiverRegistrar(int * len)
{
	return genServiceDesc(len, &scpdX_MS_MediaReceiverRegistrar);
}

static char *
genEventVars(int * len, const struct serviceDesc * s, const char * servns)
{
	const struct stateVar * v;
	char * str;
	int tmplen;
	char buf[512];
	tmplen = 512;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	*len = 0;
	v = s->serviceStateTable;
	snprintf(buf, sizeof(buf), "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\" xmlns:s=\"%s\">", servns);
	str = strcat_str(str, len, &tmplen, buf);
	while(v->name) {
		if(v->itype & 0x80) {
			snprintf(buf, sizeof(buf), "<e:property><%s>", v->name);
			str = strcat_str(str, len, &tmplen, buf);
			//printf("<e:property><s:%s>", v->name);
			switch(v->ieventvalue) {
			case 0:
				break;
			case 255:	/* Magical values should go around here */
				if( strcmp(v->name, "SystemUpdateID") == 0 )
				{
					snprintf(buf, sizeof(buf), "%d", updateID);
					str = strcat_str(str, len, &tmplen, buf);
				}
				break;
			default:
				str = strcat_str(str, len, &tmplen, upnpallowedvalues[v->ieventvalue]);
				//printf("%s", upnpallowedvalues[v->ieventvalue]);
			}
			snprintf(buf, sizeof(buf), "</%s></e:property>", v->name);
			str = strcat_str(str, len, &tmplen, buf);
			//printf("</s:%s></e:property>\n", v->name);
		}
		v++;
	}
	str = strcat_str(str, len, &tmplen, "</e:propertyset>");
	//printf("</e:propertyset>\n");
	//printf("\n");
	//printf("%d\n", tmplen);
	str[*len] = '\0';
	return str;
}

char *
getVarsContentDirectory(int * l)
{
	return genEventVars(l,
                        &scpdContentDirectory,
	                    "urn:schemas-upnp-org:service:ContentDirectory:1");
}

char *
getVarsConnectionManager(int * l)
{
	return genEventVars(l,
                        &scpdConnectionManager,
	                    "urn:schemas-upnp-org:service:ConnectionManager:1");
}

char *
getVarsX_MS_MediaReceiverRegistrar(int * l)
{
	return genEventVars(l,
                        &scpdX_MS_MediaReceiverRegistrar,
	                    "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1");
}

