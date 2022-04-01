# Step for repeating experiments from paper submission

This README explains the steps to perform the experiments from the NetSoft '22 paper submission.  

NOTE: Paper will be uploaded to arxiv soon


## Prerequisites:

1) Ensure you are on the 'netsoft' branch to match the settings used for the paper.

1) To match the paper's experiments this requires 2 VM's running Layer 4.5 framework

    * The Vagrantfile creates a server and client VM with required packages installed.

1) Various aliases are used and are pre-installed on the Vagrant VM's

    * see setup.sh for VM aliases




## NCO/DCA overhead experiment:

Purpose: This experiment tests the overhead of distributing customizations unique to each host and the associated cost of managing the customization database.  


NOTE: This first trial of 250 hosts takes about 8 min to complete, mostly due to making a module to match each emulated host.  The complete test will take much longer, depending on the number of trials selected and build argument provided.

1) (SERVER) `cd ~/software_defined_customization/experiment_scripts/netsoft`

1) (SERVER) launch experiment script:

    * `./nco_dca_batch_experiment.sh 15 no`

        * Arg1=Number of trials to perform

            * Paper experiment used 15 trials (time to complete = 1.5+ hours)

            * Recommend, 5 trials to save some time (time to complete ~ 30 min)

        * Arg2:

            * yes=modules are built for each host on first trial.  This greatly increases the experiment time.

            * no=modules only built for first trial of 250 hosts and reused for all other trials to save time.

1) View generated graph: nco_deploy.png


1) (SERVER) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/netsoft`

    * `./cleanup.sh NCO`

1) Copy tracelog to file and refresh:

    * `tracecopy nco_dca.txt`

    * copies tracelog to ~/software_defined_customization folder


## Bulk file transfer overhead experiment:

Purpose: First we determine the overhead of adding in TCP network taps for Layer 4.5 customization, but do not apply any customization to the connection.  Then we determine the overhead of the taps with a sample customization applied to the connection that performs multiple memory copy operations.


1) Copy a large test file to the NCO directory

    * The VM uses a shared folder so you don't need to copy the image to the VM since that will be accomplished by the bash script

    * The paper uses an Ubuntu.iso file (~3GB), but any large file will work.  

    * store the file as 'overhead.iso'


1) (CLIENT) `cd ~/software_defined_customization/experiment_scripts/netsoft`

1) (CLIENT) Execute the bulk transfer script to perform all experiments and
generate the graph:

    * Update script parameters to match your device:

        * SERVER_DIR=directory with overhead.iso


    * `sudo ./bulk_experiment.sh 15`

    * Arg1=Number of trials to perform

        * Paper experiment used 15 trials (time to complete = 20+ min)

        * Recommend, 5 trials to save some time (time to complete ~ 7 min)


    * View generated graph: bulk_overhead.png


1) (CLIENT) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/netsoft`

    * `./cleanup.sh BULK`



1) Copy tracelog to file and refresh:

    * `tracecopy bulk_transfer.txt`

    * copies tracelog to ~/software_defined_customization folder



## Batch DNS overhead experiment:

Purpose: First we determine the overhead of adding in UDP network taps for Layer 4.5 customization, but do not apply any customization to the connection.  Then we determine the overhead of the taps with a sample customization applied to the connection that alters each message sent.


NOTE: during this experiment all DNS queries will result in the same IP address resolution, which makes normal internet usage not possible within the VM until experiment finishes.

1) (CLIENT) `cd ~/software_defined_customization/experiment_scripts/netsoft`


1) (CLIENT) Execute the batch dns script to perform all experiments and
generate the graph:

    * `sudo ./batch_experiment.sh 15 1000 0`

        * arg1 = Number of trials

        * arg2 = Number of DNS requests in each trial

        * arg3 = Time between DNS requests

    * View generated graph: batch_overhead.png


1) (CLIENT) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/netsoft`

    * `./cleanup.sh BATCH`


1) Copy tracelog to file and refresh:

    * `tracecopy batch_dns.txt`

    * copies tracelog to ~/software_defined_customization folder



## Challenge/Response prototype:

Purpose: This demonstrates the ability to alter customization modules to include security code prior to building the modules.  Once deployed, the server can challenge the module based on the added code.


NOTE: The script assumes NCO and DCA are on same machine, but this is not a strict requirement and can be adapted to have them be different machines


1) (SERVER) Verify server VM has correct crypto driver present:

    * `cat /proc/crypto | grep cbc`

        * Response should include: "driver: cbc-aes-aesni"

    * If correct driver not present, then update 'nco_challenge_response.c' with an aes-cbc driver present on your machine

        * location: software_defined_customization/NCO/core_modules

        * skcipher = crypto_alloc_skcipher("YOUR_AES_CBC_ALGO", 0, 0);

            * example: "cbc(aes)"


1) (SERVER) `cd ~/software_defined_customization/experiment_scripts/netsoft`


1) (SERVER) Execute the shell script to conduct test:

    * `sudo ./challenge_response.sh 5 5 65`

    * arg1 = security window

    * arg2 = query interval

    * arg3 = test runtime


1) Verify NCO and DCA are running in separate terminal windows

    * First message in terminal displays either NCO or DCA

1) Verify DCA connected to NCO

    ![](../assets/connect.png)


1) Verify challenge module deployed to DCA and challenge/response window set to 5 seconds

    * NOTE: scripted experiment DCA id will always be 1

    * Open cib.db with DB Browser for SQLite (already installed): `sqlitebrowser ~/software_defined_customization/NCO/cib.db`


   ![](../assets/active_table.png)

1) Allow script to run until completed

    * Verify each check passed via terminal output

   ![](../assets/challenge.png)

   * If challenge/response fails, a message will be printed at NCO and module will be revoked

   ![](../assets/challenge_error.png)

1) Verify each challenge/response was conducted correctly by reviewing
tracelog entries

    * `tracelog`

   ![](../assets/challenge_log.png)

1) Terminate the DCA and NCO terminals when finished


1) (SERVER) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/netsoft`

    * `./cleanup.sh CHALLENGE`


1) Copy tracelog to file and refresh:

    * `tracecopy chellenge_response.txt`

    * copies tracelog to ~/software_defined_customization folder




## Middlebox demo:

Purpose: This demonstrates that a customization module can have a corresponding inverse module deployed to a network middlebox conducting deep packet inspection in order to process the customization.

NOTE: this demo will assume the middlebox is on same machine as the DNS server


1) (SERVER) `cd ~/software_defined_customization/experiment_scripts/netsoft`


1) (SERVER) Execute the shell script to conduct demo:

    * `sudo ./middle_demo.sh 10`

    * arg1 = query interval


1) Verify server NCO, DCA, and middlebox DCA are running in separate terminal windows.  DNSMASQ and tcpdump will also be running in terminal windows.

    * NOTE: server DCA id will always be 1; client DCA id is 2

    ![](../assets/terminals.png)

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



1) Wireshark:

    * click on a DNS packet to see Layer 4.5 processing

        * reload lua plugins if not seeing app tags on DNS packets in next step

    * right click on Application ID field and add column to see in main view

        ![](../assets/demo_column.png)


1) (SERVER) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/netsoft`

    * `./cleanup.sh MIDDLEBOX`


1) Copy tracelog to file and refresh:

    * `tracecopy middle_demo.txt`

    * copies tracelog to ~/software_defined_customization folder
