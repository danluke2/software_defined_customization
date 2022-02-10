# Experiments from Paper Submission HOWTO

This README explains the steps to perform the experiments from the NetSoft 22
paper submission.


1) Requires 2 VM's running Layer 4.5 framework

    * These VM's need a method to communicate over the network.  Paper used an
    intnet setup.

1) Modules were built and installed on each VM, but the NCO could also be used
to build remotely and install via DCA connection

1) Data collection and display needs to be more automated to make easier to repeat
tests






## Steps to perform bulk file transfer overhead experiment:


1) (Server) download Ubuntu image file

    * (alternatively) use any large file


1) (Server) make and install the server customization module:

    * make BUILD\_MODULE=overhead\_test\_bulk\_file\_server.o

    * sudo insmod overhead\_test\_bulk\_file\_server.ko


1) (Server) launch a simple python3 web server hosting the large file

    * client/server folder supplies a simple server with PUT capability added



1) (Client) make and install the client customization module:

    * make BUILD\_MODULE=overhead\_test\_bulk\_file\_client.o

    * sudo insmod overhead\_test\_bulk\_file\_client.ko  


1) (Client) run the bulk transfer script to perform tests

    * ./bulk\_transfer.sh 15

    * performs 15 trials

    * NOTE: update file name and md5sum in the file to match your file

    * record batch times printed to terminal

    * insert times into box.py file (tcp data lists)



## Steps to perform batch DNS overhead experiment:


1) (Server) Install dnsmasq on one of the Layer 4.5 capable VM

    * https://computingforgeeks.com/install-and-configure-dnsmasq-on-ubuntu/

    * DNSSEC not used during experiments

    * (Optional to match paper setup) Configure dnsmasq to resolve all IP addresses to same IP address


1) (Server) make and install the server customization module:

    * make BUILD\_MODULE=overhead\_test\_batch\_dns\_server.o

    * sudo insmod overhead\_test\_batch\_dns\_server.ko


1) (Server) enable dnsmasq service

    * recommend running from command line to view dnsmasq logs

    * sudo dnsmasq --no-daemon -c 0


1) (Client) make and install the client customization module:

    * make BUILD\_MODULE=overhead\_test\_batch\_dns\_client.o

    * sudo insmod overhead\_test\_batch\_dns\_client.ko  


1) (Client) run the batch DNS script to perform tests

    * ./batch\_dns.sh 15 1000

    * performs 15 trials with 1000 DNS queries

    * record batch times printed to terminal

    * insert times into box.py file (udp data lists)
