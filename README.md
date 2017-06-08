# Compile
## fix dbus-devel BUG:
```
cp /usr/lib64/dbus-1.0/include/dbus/dbus-arch-deps.h /usr/include/dbus-1.0/dbus/dbus-arch-deps.h
```
## Make
```
cd dnsmasq-with-purge && make
make install
```
 
## Config
### modify Dbus config:
```
cp dbus/dnsmasq.conf /etc/dbus-1/system.d/
service messagebus restart
```

## Start dnsmasq
```
dnsmasq --enable-dbus --enable-radmin
```

# Do purge
## Local 
```
dnsmasq_purge name
``` 
## Remote
```
echo 'REQ: PurgeCacheEntry domain_name' | nc pdnsmasq_ip 19180
```
