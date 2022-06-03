# Step for repeating buffer experiments

This README explains the steps to perform the experiments from the NetSoft '22 but using the flexible model that allows application message buffering.  



## Prerequisites:

1) Ensure you are on the 'buffering' branch 

1) To match the paper's experiments this requires 2 VM's running Layer 4.5 framework

    * The Vagrantfile creates a server and client VM with required packages installed.

1) Various aliases are used and are pre-installed on the Vagrant VM's

    * see setup.sh for VM aliases






## Bulk file transfer using HTTP over TCP overhead experiment:

Purpose: First we determine the overhead of adding in TCP network taps for Layer 4.5 customization, but do not apply any customization to the connection.  Then we determine the overhead of the taps with a sample customization applied to the connection that performs multiple memory copy operations.


1) Copy a large test file to the NCO directory

    * The VM uses a shared folder so you don't need to copy the image to the VM since that will be accomplished by the bash script

    * The paper uses an Ubuntu.iso file (~3GB), but any large file will work.  

    * store the file as 'overhead.iso'


1) (CLIENT) `cd ~/software_defined_customization/experiment_scripts/buffered`

1) (CLIENT) Execute the bulk transfer script to perform all experiments and
generate the graph:

    * Update script parameters to match your device:

        * SERVER_DIR=directory with overhead.iso


    * `sudo ./bulk_experiment_buffering.sh 15`

    * Arg1=Number of trials to perform

        * Paper experiment used 15 trials (time to complete = 20+ min)

        * Recommend, 5 trials to save some time (time to complete ~ 7 min)


    * View generated graph: buffer_bulk_overhead.png


1) (CLIENT) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/buffered`

    * `./cleanup.sh BULK`



1) Copy tracelog to file and refresh:

    * `tracecopy buffer_bulk_transfer.txt`

    * copies tracelog to ~/software_defined_customization folder



## Encrypted bulk file transfer using HTTPS over TCP overhead experiment:

Purpose: First we determine the overhead of adding in TCP network taps for Layer 4.5 customization, but do not apply any customization to the connection.  Then we determine the overhead of the taps with a sample customization applied to the connection that performs multiple memory copy operations.


1) Copy a large test file to the NCO directory

    * The VM uses a shared folder so you don't need to copy the image to the VM since that will be accomplished by the bash script

    * The paper uses an Ubuntu.iso file (~3GB), but any large file will work.  

    * store the file as 'overhead.iso'


1) (CLIENT) `cd ~/software_defined_customization/experiment_scripts/buffered`

1) (CLIENT) Execute the bulk transfer script to perform all experiments and
generate the graph:

    * Update script parameters to match your device:

        * SERVER_DIR=directory with overhead.iso


    * `sudo ./bulk_experiment_buffering_tls.sh 15`

    * Arg1=Number of trials to perform

        * Paper experiment used 15 trials (time to complete = 20+ min)

        * Recommend, 5 trials to save some time (time to complete ~ 7 min)


    * View generated graph: buffer_tls_bulk_overhead.png


1) (CLIENT) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/buffered`

    * `./cleanup.sh TLS`



1) Copy tracelog to file and refresh:

    * `tracecopy buffer_tls_bulk_transfer.txt`

    * copies tracelog to ~/software_defined_customization folder


## Batch DNS overhead experiment:

Purpose: First we determine the overhead of adding in UDP network taps for Layer 4.5 customization, but do not apply any customization to the connection.  Then we determine the overhead of the taps with a sample customization applied to the connection that alters each message sent.


NOTE: during this experiment all DNS queries will result in the same IP address resolution, which makes normal internet usage not possible within the VM until experiment finishes.

1) (CLIENT) `cd ~/software_defined_customization/experiment_scripts/buffered`


1) (CLIENT) Execute the batch dns script to perform all experiments and
generate the graph:

    * `sudo ./batch_experiment_buffering.sh 15 1000 0`

        * arg1 = Number of trials

        * arg2 = Number of DNS requests in each trial

        * arg3 = Time between DNS requests

    * View generated graph: buffer_batch_overhead.png


1) (CLIENT) Cleanup steps:

    * `cd ~/software_defined_customization/experiment_scripts/buffered`

    * `./cleanup.sh BATCH`


1) Copy tracelog to file and refresh:

    * `tracecopy buffer_batch_dns.txt`

    * copies tracelog to ~/software_defined_customization folder

