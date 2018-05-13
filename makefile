all:
	gcc simpletun.c -o simpletun
	gcc client.c -o clientWorker -lssl -lcrypto
	gcc server.c -o serverWorker -lssl -lcrypto

serverSimpletun:
	sudo ./simpletun -i tun0 -g $(ip) -d

clientSimpletun:
	sudo ./simpletun -i tun0 -c $(ip) -d

server:
	sudo ./serverWorker -i tun0 -e $(ip) -d

client:
	sudo ./clientWorker -i tun0 -c $(ip) -d

tun0server:
	sudo ip addr add 10.0.4.1/24 dev tun0
	sudo ifconfig tun0 up
	sudo route add -net 10.0.5.0 netmask 255.255.255.0 dev tun0

tun0client:
	sudo ip addr add 10.0.5.1/24 dev tun0
	sudo ifconfig tun0 up
	sudo route add -net 10.0.4.0 netmask 255.255.255.0 dev tun0

clean:
	rm -f serverWorker clientWorker simpletun