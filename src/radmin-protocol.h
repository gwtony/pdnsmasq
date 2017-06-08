#ifndef RADMIN_PROTOCOL_H
#define RADMIN_PROTOCOL_H

/*
"    <method name=\"ClearCache\">\n"
"    </method>\n"
"    <method name=\"PurgeCacheEntry\">\n"
"      <arg name=\"fqdn\" direction=\"in\" type=\"s\"/>\n"
"      <arg name=\"result\" direction=\"out\" type=\"b\"/>\n"
"    </method>\n"
"    <method name=\"GetVersion\">\n"
"      <arg name=\"version\" direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"    <method name=\"SetServers\">\n"
"      <arg name=\"servers\" direction=\"in\" type=\"av\"/>\n"
"    </method>\n"
"    <signal name=\"DhcpLeaseAdded\">\n"
"      <arg name=\"ipaddr\" type=\"s\"/>\n"
"      <arg name=\"hwaddr\" type=\"s\"/>\n"
"      <arg name=\"hostname\" type=\"s\"/>\n"
"    </signal>\n"
"    <signal name=\"DhcpLeaseDeleted\">\n"
"      <arg name=\"ipaddr\" type=\"s\"/>\n"
"      <arg name=\"hwaddr\" type=\"s\"/>\n"
"      <arg name=\"hostname\" type=\"s\"/>\n"
"    </signal>\n"
"    <signal name=\"DhcpLeaseUpdated\">\n"
"      <arg name=\"ipaddr\" type=\"s\"/>\n"
"      <arg name=\"hwaddr\" type=\"s\"/>\n"
"      <arg name=\"hostname\" type=\"s\"/>\n"
"    </signal>\n"
"  </interface>\n"
*/

#define DEFAULT_RADMIN_ADDR		"0.0.0.0"
#define	DEFAULT_RADMIN_PORT		"19180"

#define	FMT_PREFIX_REQ			"REQ: "
#define	FMT_PREFIX_RPL			"RPL: "

#define	CRNL				"\r\n"
enum {
	RADMIN_ClearCache=1,
#define STR_ClearCache			"ClearCache"
#define	FMT_ClearCache_REQ		FMT_PREFIX_REQ STR_ClearCache
	RADMIN_PurgeCacheEntry,
#define STR_PurgeCacheEntry		"PurgeCacheEntry"
#define	FMT_PurgeCacheEntry_REQ		FMT_PREFIX_REQ STR_PurgeCacheEntry " %s"
#define	FMT_PurgeCacheEntry_RPL		FMT_PREFIX_RPL "Result %lld"
	RADMIN_GetVersion,
#define	STR_GetVersion			"GetVersion"
#define	FMT_GetVersion_REQ		FMT_PREFIX_REQ STR_GetVersion
#define	FMT_GetVersion_RPL		FMT_PREFIX_RPL "Version %s"
	RADMIN_SetServers,
	RADMIN_DhcpLeaseAdded,
	RADMIN_DhcpLeaseDeleted,
	RADMIN_DhcpLeaseUpdated
};

#endif

