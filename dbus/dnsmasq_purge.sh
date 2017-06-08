#!/bin/sh

eval `dbus-launch --sh-syntax`

if [ x$1 = "x" ] ; then
	echo "Usage: $0 fqdn" >&2
	exit 1
fi

dbus-send --system --type=method_call --print-reply --dest='uk.org.thekelleys.dnsmasq' /uk/org/thekelleys/dnsmasq   uk.org.thekelleys.dnsmasq.PurgeCacheEntry string:$1

