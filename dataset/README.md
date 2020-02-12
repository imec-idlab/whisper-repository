# How to configure the set-up's of "Extending Network Programmability to the Things Overlay using Distributed Industrial IoT Protocols"
Here we are using existing Fed4FIRE+ testbeds, specifically, Citylab in Antwerp for the IoT segment and the Virtual Wall 2 in Ghent. If you have already you own testbeds, skip this part!

<u>Preparing the testbeds</u>

* Create an account in Fed4FIRE+: https://www.fed4fire.eu/run-your-experiment/
* Install and start two experiments: jFed https://www.fed4fire.eu/tutorials/
* One experiment should use Wireless Nodes (e.g., in our case 5 Wireless nodes from Citylab) for the IoT network. Pick nodes according to their location and be sure they are in range in their frecuency band (e.g., in our case 2.4 GHz). The other experiment should use Physical Nodes or Virtual Machines (e.g., 7 Phy Nodes from Virtual Wall).


# IoT segment:
-------

**Clone the Whisper repositoy in each node:**

> git clone --recursive https://github.com/imec-idlab/whisper-repository

Alternatively, you can also clone the subrepositories separately:

* For the OpenWSN nodes. Whisper has been tested up to the 'faced32ae8' commit from Dec 9, 2019:

> git clone https://github.com/openwsn-berkeley/openwsn-fw/tree/faced32ae8467678f8538b4c3de147cd81758132

* For the Whisper nodes and the root node. Whisper has been tested up to the '1ee617b65e' commit from Dec 12, 2019:

> git clone https://github.com/imec-idlab/openwsn-fw-whispernode/tree/1ee617b65ee0d97fe6d1e10727e40bd42841053c

* For the Whisper controller in the root node (Whisper has been tested up to the '1855ab3534' commit of the Whisper Controller repository from Aug 31, 2017 and up to the '62be96c8e6' commit of CoAP OpenWSN repository from Aug 31, 2017):

> git clone https://github.com/imec-idlab/whisper-controller/tree 1855ab35341a32c3b7f799131599f3c4104dee9e

> git clone https://github.com/openwsn-berkeley/coap/tree/62be96c8e636c0be748aa4633ab5626a4ad38d73

**Flash the firmware in the nodes:**

In the Whisper node and root node:
> sudo scons board=openmote-cc2538 toolchain=armgcc bootload=/dev/ttyUSB1 apps=whisper oos_openwsn

In any other node (For the normal nodes, you should create a dumb application to send pata packets to the root. Alternatively you can just use the uinject app):
> sudo scons board=openmote-cc2538 toolchain=armgcc bootload=/dev/ttyUSB2 oos_openwsn

**Configure the GRE tunnel in the IoT segment side:**

>sudo ip link add gretap1 type ip6gretap local localIPv6Address remote remoteIPv6Address

# Wired segment:
-------

**Pick the node where the ONOS controller will be installed:**

Prepare the nodes:

>sudo add-apt-repository ppa:linuxuprising/java

>sudo apt-get update

>sudo apt-get upgrade

>wget https://github.com/bazelbuild/bazel/releases/download/0.23.2/bazel-0.23.2-installer-linux-x86_64.sh

>sudo apt-get install pkg-config zip g++ zlib1g-dev python3 git curl unzip python  bzip2 openjdk-8-jdk

>sudo apt-get install openvswitch-switch

>chmod +x bazel-0.23.2-installer-linux-x86_64.sh

>./bazel-0.23.2-installer-linux-x86_64.sh --user

Add public IP (so that you can access ONOS from outside of Fed4FIRE+):

> wget -O - -q https://www.wall2.ilabt.iminds.be/enable-public-ipv4.sh | sudo bash

**Pick the node that will act as OpenFlow-enabled switches:**

Prepare the nodes:

>sudo apt-get update

>sudo apt-get upgrade

>sudo apt-get install pkg-config zip g++ zlib1g-dev python3 git curl unzip python bzip2 openjdk-8-jdk

>sudo apt-get install openvswitch-switch

Add the interfaces to the OVS bridge and set controller (i.e., 192.168.3.2 Controller's ip):

>sudo ovs-vsctl add-br br0

>sudo ifconfig X 0

>sudo ovs-vsctl add-port br0 ethX

>sudo ovs-vsctl list-ports br0

>sudo ovs-vsctl set-fail-mode br0 secure

>sudo ovs-vsctl set-controller br0 tcp:192.168.3.2:6653

Add the GRE tunnel to the switch that will be connected with the IoT network and add it to the OVS bridge:

>sudo ip link add gretap1 type ip6gretap local localIPv6Address remote remoteIPv6Address

>sudo ovs-vsctl add-port br0 gretap1

**Configure ONOS:**

Run ONOS:

>bazel run onos-local -- clean debug

Open secure connections for the REST communication between ONOS and the Whisper Controllers:

>ssh -L8181:127.0.0.1:8181 -i '/users/username/key.pem' ipDest

>ssh -L9999:127.0.0.1:9999 -i '/users/username/key.pem' ipDest

Activate apps and configure ONOS:

> /opt/onos/bin/onos -user onos

> app activate openflow-base

> app activate fwd

> app activate lldpprovider

> app activate hostprovider

> app activate proxyarp

> app activate whisper

>cfg set org.onosproject.provider.host.impl.HostLocationProvider requestIpv6ND true

Finally, you will be able to modify the paths e.g., through the CLI Whisper interface:

> /opt/onos/bin/onos -user onos

> whisper change-parent targetNode parentNode

