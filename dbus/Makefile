CFLAGS+=-I/usr/include/dbus-1.0/ -I../src/ -g -O0
LDFLAGS+=-L/usr/lib/x86_64-linux-gnu/ -ldbus-1 -rdynamic

TARGETS=dnsmasq_purge dnsmasq_radmin_helper

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

