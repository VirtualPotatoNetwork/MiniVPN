all:
	gcc simpletun.c -o simpletun
	gcc client.c -o clientWorker -lssl -lcrypto
	gcc server.c -o serverWorker -lssl -lcrypto

serverSimpletun:
	sudo ./simpletun -i tun0 -g $(ip) -d

clientSimpletun:
	sudo ./simpletun -i tun0 -c $(ip) -d

server:
	sudo ./serverWorker -i tun0 -n tun1 -s -e $(ip) -d

client:
	sudo ./serverWorker -i tun0 -n tun1 -c -e $(ip) -d

tunserver:
	sudo ip addr add 10.0.4.1/24 dev tun0
	sudo ifconfig tun0 up
	sudo ip addr add 10.0.5.1/24 dev tun1
	sudo ifconfig tun1 up

tun0client:
	sudo ip addr add 10.0.5.1/24 dev tun0
	sudo ifconfig tun0 up
	sudo route add -net 10.0.4.0 netmask 255.255.255.0 dev tun0

clean:
	rm -f serverWorker clientWorker simpletun