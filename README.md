# Gateway-to-Gateway Virtual Private Network
This project is a simple

## How to run
To test this VPN properly, you need two different computers with a Virtual Machine Application ([VirtualBox](https://www.virtualbox.org/) is recommended).

If you follow all the example settings you will probably be fine.

---

### VirtualBox settings

You need two gateways and two hosts for testing. In each computer, open two virtual machines with Ubuntu. One of these virtual machines will act as a gateway and the other as a host in each computer.

Follow each step:

**1. Gateway virtual machine settings**

Do the following instructions on each computer:

* Open settings of the **gateway** virtual machine

* Open `Network` tab

* Open `Adapter 2` tab

* Create an internal network

<img src="/readme-media/gateway-settings.png" alt="Gateway Settings" style="width: 460px;"/>


**2. Host virtual machine settings**

Do the following instructions on each computer:

* Open settings of the **host** virtual machine

* Open `Network` tab

* Open `Adapter 1` tab

* Create an internal network

<img src="/readme-media/host-settings.png" alt="Gateway Settings" style="width: 460px;"/>

**3. VirtualBox subnet configuration**

Run the following command on each computer:

```bash
vboxmanage dhcpserver add --netname testnet --ip <first-ip> --netmask 255.255.255.0 --lowerip <lower-ip> --upperip <upper-ip> --enable
```
Example:
```bash
# in the computer1, choose 10.10.10.0 as the subnet
vboxmanage dhcpserver add --netname testnet --ip 10.10.10.1 --netmask 255.255.255.0 --lowerip 10.10.10.2 --upperip 10.10.10.20 --enable

# in the computer2, choose 10.10.9.0 as the subnet
vboxmanage dhcpserver add --netname testnet --ip 10.10.9.1 --netmask 255.255.255.0 --lowerip 10.10.9.2 --upperip 10.10.9.20 --enable
```

**4. Gateway Port Forwarding**

You need to forward packets received by the real computer's `port 55555` to gateway's `port 55555` in each computer.

* Open settings of the **gateway** virtual machine

* Open `Network` tab

* Open `Adapter 1` tab

* Click `Advanced`

* Click `Port Forwarding`

* Add the rule for forwarding packets

Example:

<img src="/readme-media/port-forwarding-rule1.png" alt="Gateway1 Settings" style="width: 460px;"/>

<img src="/readme-media/port-forwarding-rule2.png" alt="Gateway2 Settings" style="width: 460px;"/>

---

### Routing and TUN Interface Configurations

All default routing and TUN interface configurations are handled with the make commands.

In gateway virtual machines, in the folder our `makefile` resides:

```bash
make all
make server ip=ComputerIP
make tunserver
```
Note that `ComputerIP` is the IP of the computer the gateway virtual machine resides.

Lastly, in order to be able to route packets from TUN interface to the host in private network you need to run the following commands in each gateway virtual machine:

```bash
sudo su
echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter
echo 0 > /proc/sys/net/ipv4/conf/tun1/rp_filter
```
In host virtual machines:

```bash
# TODO
```

