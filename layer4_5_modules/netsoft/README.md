# NetSoft Customization Modules


Here we include modules used in the NetSoft paper submission. See experiment_scripts/netsoft for the tests performed with these modules.


* nco\_overhead\_exp: Generic module that is transferred between the NCO and each DCA for testing overhead of customization management at the NCO.

    * could be any module since simulated insertion at each DCA



* overhead\_test\_batch\_dns: Inserts a DNS tag to the front of each DNS query performed using the `dig` application


* overhead\_test\_bulk\_dns:: Inserts a tag every X bytes of a HTTP file transfer.
