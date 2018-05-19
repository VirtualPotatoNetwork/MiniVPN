# MiniVPN
Simple SSL VPN for Ubuntu

## HOW TO RUN

* 'subnet' is other private network's subnet

for server:
> make all
> make server ip=
> make tunserver subnet=

for client (internal is the internal interface name ex:enp0s3)
> make client subnet= gateway= internal=