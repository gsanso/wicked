/*
 *	DHCP6 option utilities used in addrconf / lease and supplicant
 *
 *	Copyright (C) 2010-2013 SUSE LINUX Products GmbH, Nuernberg, Germany.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, see <http://www.gnu.org/licenses/> or write
 *	to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *	Boston, MA 02110-1301 USA.
 *
 *	Authors:
 *		Olaf Kirch <okir@suse.de>
 *		Marius Tomaschewski <mt@suse.de>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <wicked/util.h>
#include "dhcp6/options.h"
#include "util_priv.h"

/*
 * status
 */
static const char *	__dhcp6_status_codes[__NI_DHCP6_STATUS_MAX] = {
	[NI_DHCP6_STATUS_SUCCESS]	= "Success",
	[NI_DHCP6_STATUS_FAILURE]	= "UnspecFail",
	[NI_DHCP6_STATUS_NOADDRS]	= "NoAddrsAvail",
	[NI_DHCP6_STATUS_NOBINDING]	= "NoBinding",
	[NI_DHCP6_STATUS_NOTONLINK]	= "NotOnLink",
	[NI_DHCP6_STATUS_USEMULTICAST]	= "UseMulticast",
};

const char *
ni_dhcp6_status_name(unsigned int code)
{
	static char namebuf[64];
	const char *name = NULL;

	if (code < __NI_DHCP6_STATUS_MAX)
		name = __dhcp6_status_codes[code];

	if (!name) {
		snprintf(namebuf, sizeof(namebuf), "[%u]", code);
		name = namebuf;
	}
	return name;
}

ni_dhcp6_status_t *
ni_dhcp6_status_new(void)
{
	return xcalloc(1, sizeof(ni_dhcp6_status_t));
}

void
ni_dhcp6_status_clear(ni_dhcp6_status_t *status)
{
	status->code = 0;
	ni_string_free(&status->message);
}

void
ni_dhcp6_status_destroy(ni_dhcp6_status_t **status)
{
	if (status && *status) {
		ni_dhcp6_status_clear(*status);
		free(*status);
		*status = NULL;
	}
}


/*
 * ia address
 */
ni_dhcp6_ia_addr_t *
ni_dhcp6_ia_addr_new(const struct in6_addr addr, unsigned int plen)
{
	ni_dhcp6_ia_addr_t *iadr;

	iadr = xcalloc(1, sizeof(*iadr));
	iadr->addr = addr;
	iadr->plen = plen;
	return iadr;
}

void
ni_dhcp6_ia_addr_destory(ni_dhcp6_ia_addr_t *iadr)
{
	ni_dhcp6_status_clear(&iadr->status);
	free(iadr);
}


/*
 * ia address list
 */
void
ni_dhcp6_ia_addr_list_append(ni_dhcp6_ia_addr_t **list, ni_dhcp6_ia_addr_t *iadr)
{
	while (*list)
		list = &(*list)->next;
	*list = iadr;
}

void
ni_dhcp6_ia_addr_list_destroy(ni_dhcp6_ia_addr_t **list)
{
	ni_dhcp6_ia_addr_t *iadr;
	while ((iadr = *list) != NULL) {
		*list = iadr->next;
		ni_dhcp6_ia_addr_destory(iadr);
	}
}


/*
 * ia
 */
ni_dhcp6_ia_t *
ni_dhcp6_ia_new(unsigned int type, unsigned int iaid)
{
	ni_dhcp6_ia_t *ia;

	ia = xcalloc(1, sizeof(*ia));
	ia->type = type;
	ia->iaid = iaid;
	return ia;
}

void
ni_dhcp6_ia_destroy(ni_dhcp6_ia_t *ia)
{
	ni_dhcp6_status_clear(&ia->status);
	ni_dhcp6_ia_addr_list_destroy(&ia->addrs);
	free(ia);
}


/*
 * ia list
 */
void
ni_dhcp6_ia_list_destroy(ni_dhcp6_ia_t **list)
{
	ni_dhcp6_ia_t *ia;
	while ((ia = *list) != NULL) {
		*list = ia->next;
		ni_dhcp6_ia_destroy(ia);
	}
}

void
ni_dhcp6_ia_list_append(ni_dhcp6_ia_t **list, ni_dhcp6_ia_t *ia)
{
	while (*list)
		list = &(*list)->next;
	*list = ia;
}

/*
 * Map DHCP6 options to names
 */
static const char *__dhcp6_option_names[__NI_DHCP6_OPTION_MAX] = {
	[NI_DHCP6_OPTION_CLIENTID]          =	"client-id",
	[NI_DHCP6_OPTION_SERVERID]          =	"server-id",
	[NI_DHCP6_OPTION_IA_NA]             =	"ia-na",
	[NI_DHCP6_OPTION_IA_TA]             =	"ia-ta",
	[NI_DHCP6_OPTION_IA_ADDRESS]        =	"ia-address",
	[NI_DHCP6_OPTION_ORO]               =	"oro",
	[NI_DHCP6_OPTION_PREFERENCE]        =	"preference",
	[NI_DHCP6_OPTION_ELAPSED_TIME]      =	"elapsed-time",
	[NI_DHCP6_OPTION_RELAY_MSG]         =	"relay-msg",
	[NI_DHCP6_OPTION_AUTH]              =	"auth",
	[NI_DHCP6_OPTION_UNICAST]           =	"unicast",
	[NI_DHCP6_OPTION_STATUS_CODE]       =	"status-code",
	[NI_DHCP6_OPTION_RAPID_COMMIT]      =	"rapid-commit",
	[NI_DHCP6_OPTION_USER_CLASS]        =	"user-class",
	[NI_DHCP6_OPTION_VENDOR_CLASS]      =	"vendor-class",
	[NI_DHCP6_OPTION_VENDOR_OPTS]       =	"vendor-opts",
	[NI_DHCP6_OPTION_INTERFACE_ID]      =	"interface-id",
	[NI_DHCP6_OPTION_RECONF_MSG]        =	"reconf-msg",
	[NI_DHCP6_OPTION_RECONF_ACCEPT]     =	"reconf-accept",
	[NI_DHCP6_OPTION_SIP_SERVER_D]      =	"sip-server-names",
	[NI_DHCP6_OPTION_SIP_SERVER_A]      =	"sip-server-addresses",
	[NI_DHCP6_OPTION_DNS_SERVERS]       =	"dns-servers",
	[NI_DHCP6_OPTION_DNS_DOMAINS]       =	"dns-domains",
	[NI_DHCP6_OPTION_IA_PD]             =	"ia-pd",
	[NI_DHCP6_OPTION_IA_PREFIX]         =	"ia-prefix",
	[NI_DHCP6_OPTION_NIS_SERVERS]       =	"nis-servers",
	[NI_DHCP6_OPTION_NISP_SERVERS]      =	"nisplus-servers",
	[NI_DHCP6_OPTION_NIS_DOMAIN_NAME]   =	"nis-domain",
	[NI_DHCP6_OPTION_NISP_DOMAIN_NAME]  =	"nisplus-domain",
	[NI_DHCP6_OPTION_SNTP_SERVERS]      =	"sntp-servers",
	[NI_DHCP6_OPTION_INFO_REFRESH_TIME] =	"info-refresh-time",
	[NI_DHCP6_OPTION_BCMCS_SERVER_D]    =	"bcms-domains",
	[NI_DHCP6_OPTION_BCMCS_SERVER_A]    =	"bcms-servers",
	[NI_DHCP6_OPTION_GEOCONF_CIVIC]     =	"geoconf-civic",
	[NI_DHCP6_OPTION_REMOTE_ID]         =	"remote-id",
	[NI_DHCP6_OPTION_SUBSCRIBER_ID]     =	"subscriber-id",
	[NI_DHCP6_OPTION_FQDN]              =	"fqdn",
	[NI_DHCP6_OPTION_PANA_AGENT]        =	"pana-agent",
	[NI_DHCP6_OPTION_POSIX_TZ_STRING]   =	"posix-tz-string",
	[NI_DHCP6_OPTION_POSIX_TZ_DBNAME]  =	"posix-tz-dbname",
	[NI_DHCP6_OPTION_ERO]               =	"ero",
	[NI_DHCP6_OPTION_LQ_QUERY]          =	"lq-query",
	[NI_DHCP6_OPTION_CLIENT_DATA]       =	"client-data",
	[NI_DHCP6_OPTION_CLT_TIME]          =	"clt-time",
	[NI_DHCP6_OPTION_LQ_RELAY_DATA]     =	"lq-relay-data",
	[NI_DHCP6_OPTION_LQ_CLIENT_LINK]    =	"lq-cient-link",
	[NI_DHCP6_OPTION_MIP6_HNINF]        =	"mip6-hninf",
	[NI_DHCP6_OPTION_MIP6_RELAY]        =	"mip6-relay",
	[NI_DHCP6_OPTION_V6_LOST]           =	"v6-lost",
	[NI_DHCP6_OPTION_CAPWAP_AC_V6]      =	"capwap-ac-v6",
	[NI_DHCP6_OPTION_RELAY_ID]          =	"relay-id",
	[NI_DHCP6_OPTION_MOS_ADDRESSES]     =	"mos-addresses",
	[NI_DHCP6_OPTION_MOS_DOMAINS]       =	"mos-domains",
	[NI_DHCP6_OPTION_NTP_SERVER]        =	"ntp-server",
	[NI_DHCP6_OPTION_V6_ACCESS_DOMAIN]  =	"v6-access-domain",
	[NI_DHCP6_OPTION_SIP_UA_CS_LIST]    =	"sip-ua-cs-list",
	[NI_DHCP6_OPTION_BOOTFILE_URL]      =	"bootfile-url",
	[NI_DHCP6_OPTION_BOOTFILE_PARAM]    =	"bootfile-param",
	[NI_DHCP6_OPTION_CLIENT_ARCH_TYPE]  =	"client-arch-type",
	[NI_DHCP6_OPTION_NII]               =	"nii",
	[NI_DHCP6_OPTION_GEOLOCATION]       =	"geolocation",
	[NI_DHCP6_OPTION_AFTR_NAME]         =	"aftr-name",
	[NI_DHCP6_OPTION_ERP_LOCAL_DOMAIN]  =	"erp-local-domain",
	[NI_DHCP6_OPTION_RSOO]              =	"rsoo",
	[NI_DHCP6_OPTION_PD_EXCLUDE]        =	"pd-exclude",
	[NI_DHCP6_OPTION_VSS]               =	"vss",
};

const char *
ni_dhcp6_option_name(unsigned int option)
{
	static char namebuf[64];
	const char *name = NULL;

	if (option < __NI_DHCP6_OPTION_MAX)
		name = __dhcp6_option_names[option];

	if (!name) {
		snprintf(namebuf, sizeof(namebuf), "[%u]", option);
		name = namebuf;
	}
	return name;
}
