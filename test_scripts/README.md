# Step for repeating experiments from paper submission

This README explains the steps to perform the experiments from the NetSoft '22
paper submission.  (NOTE: being updated to make easier to correlate with paper
  and repeat experiments)


1) To match the paper's experiments this requires 2 VM's running Layer 4.5 framework

    * These VM's need a method to communicate over the network.  Paper used an
    intnet setup.

    * To test functionality only, one VM can be used


1) Modules were built and installed on each VM, but the NCO could also be used
to build remotely and install via DCA connection


1) Data collection and display needs to be more automated to make easier to repeat
tests



## NCO/DCA overhead experiment:


1) (NCO) install database browser:

    * I used DB Browser for SQLite (https://sqlitebrowser.org/dl/)

    * this is just a way to visualize database and manipulate entries


1) (NCO) launch NCO test program:

    * python3 test\_NCO.py --t 60

    * this delays the build module thread 60 seconds to allow desired number of
    DCA check-ins to occur before building modules (adjust as needed)


1) (DCA\_user) launch test DCA program

    * python3 test\_DCA.py --start 0 --end 50 --dir path\_to\_temp\_dir

    * start/end: create end-start device threads with unique names and fake mac
    addresses corresponding to those names

    * dir: directory to hold all the kernel modules that will be transfered over
    the control channel


1) (NCO) After all devices have checked in and build module is waiting for user
input to start running:

    * enter "y" in terminal to build modules for each device in test

        * this can take a while depending on how many devices to test, so I
        recommend not rebuilding between trials and only when changing number
        of devices

    * record build time if interested in collecting

    * enter "n" to start the experiment

        * this notifies all waiting NCO/DCA threads that models are ready to
        deploy

    * after all modules are delivered, record time between last delivery and
    first delivery

        * NOTE: each device thread outputs a starting time and finish time so
        a lot of times are printed to the terminal

1) Stop the DCA and NCO and verify test performed on all devices:

    * (DCA) verify a kernel module was downloaded for each host

    * (NCO) verify an entry exists for each host in CIB


1) Repeat previous steps 15 times for each test (10,50,100,175,250)





## Bulk file transfer overhead experiment:


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



## Batch DNS overhead experiment:


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



## Middlebox demo:


1) Steps assume the prior overhead DNS experiment was done, but also that
the NCO is running on the Server with DCA on Client and Server

    * ensure no other modules installed on Server/Client before continuing


1) (NCO) make, deploy, and install the server customization module:

    * add 'demo\_dns\_server\_app' to CIB for server

    * add 'demo_dns_tag.lua' as module dependency to trigger deployment also


1) (NCO) make, deploy, and install the client customization module:

    * add 'demo\_dns\_client\_app' to CIB for server

    * add 'demo_dns_tag.lua' as module dependency to trigger deployment also


1) (Client/Server) Collect UDP packets:

    * using Wireshark or tcpdump on client and/or server

    * Just need to get the network point of view of the customized request


1) (Client) Perform several DNS queries

    * dig needs to be one of them

    * Others can be done with curl or web browser or ....


1) Open Wireshark at the collection point chosen above

    * verify application tag was processed for the dig request, but not others
