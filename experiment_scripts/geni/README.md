# Step for repeating GENI experiments

This README explains the steps to perform the dissertation GENI experiments.  


## Prerequisites:

1) You need a GENI account to perform all tests

    * http://www.geni.net/get-involved/get-started/

1) You need an ssh key with passphase for GENI login:

    * This passphase will be your GENI_PASSWORD for the scripts 

1) Ensure you are on the 'rotating' branch to match the settings used for the dissertation.

1) To match the dissertation's middlebox experiments this requires 1 VM running Layer 4.5 framework

    * Note: the Vagrantfile creates a server and client VM with required packages installed.

1) Various aliases are used and are pre-installed on the Vagrant VM's

    * see setup.sh for VM aliases


1) You need a publicly accessible web server to host GENI configuration files:

    1) Create web server GENI slice 

    1) Pick a server location, Xen VM running Ubuntu, and a public IP address 

    1) Log into server and install apache2

    1) Create a ``geni`` folder in ``/var/www/html``

    1) Run the tar.sh script in ``setup\_scipts`` to generate tar files

    1) Push all tar files to the web server folder: ``/var/www/html/geni``



## Web/DNS Middlebox experiment:

Purpose: This experiment tests 3rd party middlebox interference of Layer 4.5 customizations.  

1) Create two GENI slices

    * we split test into 2 batches in case of node issues

1) Upload GENI desired rspec file, but do not allocate resources yet:

    * ``middle_interference_batch_1_rspec.xml``

    * ``middle_interference_batch_2_rspec.xml``


1) Update the IP address for tar files:

    * Section "Install Tarball"
    
    * http://YOUR_IP/geni/middlebox.tar.gz

1) Update GENI username for shell file:

    * Section "Execute Command"

    * sh /local/middlebox.sh YOUR_USERNAME

1) Allocate GENI resources

    * this takes around 3 minutes, but wait at least 2 additional minutes for shell scripts to finish


1) Launch local client VM

    * Either client or server Vagrant VM will work 


1) Execute the config.sh script to update GENI username and passphase:

    * ./config.sh YOUR_USENAME YOUR_PASSPHRASE

1) Update the batch script:

    * ``middlebox_batch.sh``

    * Update the IP addresses for the allocated GENI nodes 

    * comment out tests that you won't be running (i.e., batch 2 or web or dns portions)

1) Execute batch script:

    * $1 = number of trials

    * $2 = the client IP for server modules to match against 

        * either your routers public IP address or 0.0.0.0 to match all

1) Review generated logs for test results:

    * ``experiment_scripts/logs/SERVER_IP/``

    * View PCAP files to verify tag insertions

    * View web txt files to verify hash values for file transfers 

    * View dns txt files to verify DNS response values
