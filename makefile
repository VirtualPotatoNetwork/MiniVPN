all:
	gcc simpletun.c -o simpletun
	gcc client.c -o clientWorker -lssl -lcrypto
	gcc server.c -o serverWorker -lssl -lcrypto

serverSimpletun:
	sudo ./simpletun -i tun0 -g $(ip) -d

clientSimpletun:
	sudo ./simpletun -i tun0 -c $(ip) -d

server:
	sudo ./serverWorker -i tun0 -n tun1 -e $(ip) -s -d

client:
	sudo ./serverWorker -i tun0 -n tun1 -e $(ip) -c  -d

tunserver:
	sudo ip addr add 10.0.4.1/24 dev tun0
	sudo ifconfig tun0 up
	sudo ip addr add 10.0.5.1/24 dev tun1
	sudo ifconfig tun1 up
	sudo route add -net $(subnet) gw 10.0.4.1 netmask 255.255.255.0 dev tun0
	sudo sysctl net.ipv4.ip_forward=1

client:
	sudo route add -net $(subnet) gw $(gateway) netmask 255.255.255.0 dev $(interface)

clean:
	rm -f serverWorker clientWorker simpletun