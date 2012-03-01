/*
 * DBus encapsulation for network interfaces
 *
 * Copyright (C) 2011-2012 Olaf Kirch <okir@suse.de>
 */

#include <string.h>
#include <stdlib.h>

#include <wicked/netinfo.h>
#include <wicked/logging.h>
#include <wicked/dbus-errors.h>
#include <wicked/ethernet.h>
#include "dbus-common.h"
#include "model.h"
#include "debug.h"

static unsigned int			ni_objectmodel_ns_count;
static ni_objectmodel_netif_ns_t **	ni_objectmodel_ns_list;

void
ni_objectmodel_register_netif_ns(ni_objectmodel_netif_ns_t *ns)
{
	if ((ni_objectmodel_ns_count % 8) == 0)
		ni_objectmodel_ns_list = realloc(ni_objectmodel_ns_list,
				(ni_objectmodel_ns_count + 8) * sizeof(ni_objectmodel_ns_list[0]));
	ni_objectmodel_ns_list[ni_objectmodel_ns_count++] = ns;
}

ni_objectmodel_netif_ns_t *
ni_objectmodel_get_netif_ns(const char *name)
{
	unsigned int i;

	for (i = 0; i < ni_objectmodel_ns_count; ++i) {
		ni_objectmodel_netif_ns_t *ns;

		ns = ni_objectmodel_ns_list[i];
		if (ni_string_eq(ns->name, name))
			return ns;
	}
	return NULL;
}

ni_interface_t *
ni_objectmodel_netif_by_attrs(ni_objectmodel_netif_ns_t *ns, const ni_var_array_t *attrs)
{
	ni_netconfig_t *nc = ni_global_state_handle(0);
	ni_interface_t *dev;

	if (ns->lookup_by_attrs)
		return ns->lookup_by_attrs(ns, attrs);

	if (ns->match_attr == NULL)
		return NULL;

	for (dev = ni_interfaces(nc); dev; dev = dev->next) {
		dbus_bool_t match = TRUE;
		ni_var_t *ap;
		unsigned int i;

		for (i = 0, ap = attrs->data; match && i < attrs->count; ++i, ++ap)
			match = ns->match_attr(dev, ap->name, ap->value);
		if (match) {
			ni_debug_dbus("%s: found %s", __func__, dev->name);
			return dev;
		}
	}
	return NULL;
}

static ni_bool_t
__match_hwaddr(const ni_hwaddr_t *hwaddr, const char *string)
{
	ni_hwaddr_t match;

	if (!string)
		return FALSE;
	if (ni_link_address_parse(&match, NI_IFTYPE_ETHERNET, string) < 0)
		return FALSE;

	return ni_link_address_equal(hwaddr, &match);
}

static dbus_bool_t
ni_objectmodel_ether_match_attr(const ni_interface_t *dev, const char *name, const char *value)
{
	ni_ethernet_t *eth;

	if (!(eth = dev->ethernet))
		return FALSE;

	if (!strcmp(name, "address"))
		return __match_hwaddr(&dev->link.hwaddr, value);

	if (!strcmp(name, "permanent-address"))
		return __match_hwaddr(&eth->permanent_address, value);

	ni_warn("%s: unsupported query attribute %s", __func__, name);
	return FALSE;
}

static ni_objectmodel_netif_ns_t ni_objectmodel_ether_ns = {
	.name		= "ethernet",
	.match_attr	= ni_objectmodel_ether_match_attr,
};

void
ni_objectmodel_register_netif_ns_builtin(void)
{
	ni_objectmodel_register_netif_ns(&ni_objectmodel_ether_ns);
}