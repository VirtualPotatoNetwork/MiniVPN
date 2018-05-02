all:
	gcc simpletun.c -o simpletun
server:
	sudo ./simpletun -i tun0 -s -d
conberkay:
	sudo ./simpletun -i tun0 -c 10.70.190.45 -d
tun0server:
	sudo ip addr add 10.0.4.1/24 dev tun0
	ifconfig tun0 up
	route add -net 10.0.5.0 netmask 255.255.255.0 dev tun0
tun0client:
	sudo ip addr add 10.0.5.1/24 dev tun0
	ifconfig tun0 up
	route add -net 10.0.4.0 netmask 255.255.255.0 dev tun0