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

    * sudo experiment\_scripts/buk_experiment.sh 15

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

    * sudo experiment\_scripts/batch_experiment.sh 15 1000 0

        * performs 15 trials of 1000 DNS requests with 0 sec between each request

    * View generated graph: batch_overhead.png





## Challenge/Response prototype:

1) NOTE: The script assumes NCO and DCA are on same machine, but this is not
a requirement and can be adapted to have them be different machines

1) Update NCO config file (cfg.py) to match directory path for your machine:

    * git\_dir, symvers\_dir, etc.


1) Execute the shell script to conduct test:

    * sudo ./software_defined_customization/experiment_scripts/challenge\_response.sh 5 5 65

    * arg1 = security window

    * arg2 = query interval

    * arg3 = test runtime


1) Verify NCO and DCA are running in separate terminal windows

1) Verify DCA connected to NCO

1) Verify challenge module deployed to DCA and challenge/response window set to 5 seconds

    * NOTE: scripted experiment DCA id will always be 1

    * ![](../assets/active_table.png)

1) Allow script to run until completed

    * Verify each check passed via terminal output

    * ![](../assets/challenge.png)

1) Verify each challenge/response was conducted correctly by reviewing
tracelog entries

    * ![](../assets/challenge_log.png)




## Middlebox demo:

1) NOTE: this demo will assume the middlebox is on same machine as the DNS server

1) Update NCO config file (cfg.py) to match directory path for your machine:

    * git\_dir, symvers\_dir, etc.


1) Execute the shell script to conduct demo:

    * sudo ./software_defined_customization/experiment_scripts/middle\_demo.sh 10

    * arg21 = query interval


1) Verify server NCO, DCA, and middlebox DCA are running in separate terminal windows.  DNSMASQ and tcpdump will also be running in terminal windows.

    * NOTE: server DCA id will always be 1; client DCA id is 2

    * ![](../assets/demo_column.png)

    * Top-left to Bottom-right:

        * shell script window

        * NCO

        * dnsmasq

        * server DCA

        * middlebox DCA

        * tcpdump

1) Verify all DCA's connected to NCO. Client DCA is host_id 2 and should show connected in NCO terminal also.

1) Verify demo modules deployed to server/client DCA and invervse module sent to middlebox DCA


1) Allow script to run until completed, signaled by Wireshark opening collected traffic packet.

  * dnsmasq terminal will stay open for verification that dns queries were made



1) Wireshark:

    * click on a DNS packet to see Layer 4.5 processing

        * reload lua plugins if not seeing app tags on DNS packets in next step

    * right click on Application ID field and add column to see in main view

        * ![](../assets/demo_column.png)
