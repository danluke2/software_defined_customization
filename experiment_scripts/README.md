# Step for repeating experiments from paper submission

This README explains the steps to perform the experiments from the NetSoft '22
paper submission.  

NOTE: being updated to make easier to correlate with paper
  and repeat experiments


Note: Modules were built and installed on each VM, but the NCO could also be used
to build remotely and install via DCA connection





## Prerequisites:
1) To match the paper's experiments this requires 2 VM's running Layer 4.5 framework

    * These VM's need a method to communicate over the network.  Paper used an
    intnet setup.

    * download git repo to each VM (Client and Server)


1) (Client/DCA VM) Install sshpass and curl to allow remote server login and command execution

    * sudo apt install sshpass curl


1) (Client/DCA VM) Install matplotlib for plotting graphs:

    * pip install matplotlib


1) (Server/NCO VM) Install ssh server, sshpass, and enable root login:

    * sudo apt install openssh-server sshpass

    * open /etc/ssh/sshd\_config

    * add 'PermitRootLogin yes'

    * restart ssh: sudo systemctl restart ssh

1) (Both VM) set root password:

    * sudo passwd root

    * set to 'default'


1) (Client/DCA) ssh to Server VM to establish key:

    * sudo ssh root@10.0.0.20

    * ssh root@10.0.0.20

    * password = 'default'


1) (Server/NCO) ssh to Client VM to establish key:

    * sudo ssh root@10.0.0.40

    * ssh root@10.0.0.40

    * password = 'default'


1) (NCO) (Optional) install database browser:

    * I used DB Browser for SQLite (https://sqlitebrowser.org/dl/)

    * this is just a way to visualize database and manipulate entries




## NCO/DCA overhead experiment:


1) (NCO) launch experiment script:

    * update nco\_dca\_(batch)\_experiment files with your parameters

    * run: nco\_dca\_batch\_experiment 15

        * this performs 15 trials for each number of hosts and plots results

1) View generated graph: nco_deploy.png



## Bulk file transfer overhead experiment:


1) (Server) download Ubuntu image file

    * (alternatively) use any large file

    * store the file as 'overhead.iso' on the Desktop


1) (Client) Execute the bulk transfer script to perform all experiments and
generate the graph:

    * Update script parameters to match your device:

        * SERVER_DIR=directory with overhead.iso

        * GIT_DIR=directory holding the software_defined_customization git repo

        * SIMPLE_SERVER_DIR=$GIT_DIR/experiment_scripts/client_server

    * test\_scripts/buk_experiment.sh 15

    * performs 15 trials

    * View generated graph: bulk_overhead.png



## Batch DNS overhead experiment:


1) (Server) Install dnsmasq on one of the Layer 4.5 capable VM

    * https://computingforgeeks.com/install-and-configure-dnsmasq-on-ubuntu/

    * DNSSEC not used during experiments

    * (Optional to match paper setup) Configure dnsmasq to:

        * resolve all IP addresses to same IP address

        * log queries

        * no cache


1) (Client) Execute the batch dns script to perform all experiments and
generate the graph:

    * Update script parameters to match your device:

        * GIT_DIR=directory holding the software_defined_customization git repo

    * test\_scripts/batch_experiment.sh 15 1000 0

        * performs 15 trials of 1000 DNS requests with 0 sec between each request

    * View generated graph: batch_overhead.png





## Challenge/Response prototype:

1) (NCO) Start NCO to begin listening for DCA connections

    * python3 software_defined_customization/NCO/NCO.py --challenge --window 5

1) (DCA) Start DCA and verify connected to NCO

    * sudo python3 software_defined_customization/DCA\_user/DCA.py

    * sudo required to install and remove modules

1) (NCO) make, deploy, and install the experiment module:

    * add 'nco\_challenge\_response' to CIB build table for connected DCA device

1) (NCO) Verify nco\_challenge\_response is built and deployed to DCA and challenge/response
window set to 5 seconds

1) (NCO) Allow NCO to run until 20 challenge/responses have completed

    * Verify each check passed via terminal output

    * TODO: add image here

1) (DCA) Verify each challenge/response was conducted correctly by reviewing
tracelog entries

    * TODO: add image here




## Middlebox demo:


1) Steps assume the prior overhead DNS experiment was done, but also that
the NCO is running on the Server with DCA on Client and Server

    * ensure no other modules installed on Server/Client before continuing


1) (NCO) make, deploy, and install the server customization module:

    * add 'demo\_dns\_server\_app\_tag' to CIB for server

    * add 'demo_dns_tag.lua' as module dependency to trigger deployment also


1) (NCO) make, deploy, and install the client customization module:

    * add 'demo\_dns\_client\_app\_tag' to CIB for server

    * add 'demo_dns_tag.lua' as module dependency to trigger deployment also


1) (Client/Server) Collect UDP packets:

    * using Wireshark or tcpdump on client and/or server

    * Just need to get the network point of view of the customized request


1) (Client) Perform several DNS queries

    * dig needs to be one of them

    * Others can be done with curl or web browser or ....


1) Open Wireshark at the collection point chosen above

    * verify application tag was processed for the dig request, but not others
