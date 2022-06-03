# Sample Customization Modules

NOTE: This README only uses one of the Vagrant VM's created.  It does not matter which one you use (client or server)


Here we include sample modules to use with the Layer 4.5 installation.  These modules have been adapted from Main branch to use the flexible Layer 4.5 model that can buffer data for the application.


* buffer\_python\_client: Inserts a tag to the front of the message sent to
the server.

    * Example:

        * (Client) Hello

        * (Server) testCustModHello

    * Matches the flows:

      * source IP/port: \*/\*

      * dest IP/port: 127.0.0.1/65432

      * L4 protocol: TCP or UDP

      * Application: python3


* buffer\_python\_server: Removes the tag at the front of the message sent from
the client.

    * NOTE: This module assumes the client module is loaded.  If loaded without
    the client module, then kernel may crash.

    * Example:

        * (Client) Hello

        * (Network) testCustModHello

        * (Server) Hello

    * Matches the flows:

      * source IP/port: 127.0.0.1/65432

      * dest IP/port: \*/\*

      * L4 protocol: TCP or UDP

      * Application: python3    


* active\_buffer\_python\_server: Removes the tag at the front of the message sent from
the client, but will also buffer the last 5 bytes of the message if the size is larger than 5 bytes.  The following message will have the buffered data inserted to the front of the message.

    * NOTE: This module assumes the client module is loaded.  If loaded without
    the client module, then kernel may crash.

    * Example:

        * (Client) HelloXXXXX

        * (Network) testCustModHelloXXXXX

        * (Server) Hello

        * (Client) Hello

        * (Network) testCustModHello

        * (Server) XXXXXHello

    * Matches the flows:

      * source IP/port: 127.0.0.1/65432

      * dest IP/port: \*/\*

      * L4 protocol: TCP or UDP

      * Application: python3 



## Steps to run sample client and server customizations:

1) All steps run on a single VM, either client or server VM will work

1) `cd ~/software_defined_customization/layer4_5_modules/sample_modules`

1) Make the sample python client and server kernel module:

    * BUILD\_MODULE is command line arg to direct building a specific module

    * `make BUILD_MODULE=buffer_python_client.o`

    * `make BUILD_MODULE=active_buffer_python_server.o`

        * verify no errors during module build



1) Insert client module:

    * `sudo insmod buffer_python_client.ko`

    * Verify client module loaded messages are present in trace log

        * `tracelog`


1) insert server module:

    * `sudo insmod active_buffer_python_server.ko`

    * Verify client and server module loaded messages are present in trace log

        * `tracelog`



1) Launch the python echo client and server in two separate terminals and note the PID of each (printed in terminal):

    * Python code location: experiment_scripts/client\_server

    * `server_echo --tcp` (--udp)

    * `client_echo --tcp` (--udp)


1) In a new terminal window, launch tcpdump to verify changes are applied to messages:

    * `sudo tcpdump port 65432 -i lo -X`

    * alternatively, launch Wireshark and choose loopback interface



1) type some messages into the echo client and verify tcpdump shows modified messages

    * echo client/server should show un-modified messages (unless buffering happened)


1) In the client terminal, type 'quit' to close the connection


1) dump the kernel trace file to find corresponding messages for layer 4.5 messages:

    * `tracelog`

    * cust\_send=1 means send customization will be applied on the client

    * cust\_recv=1 means recv customization will be applied on the server



1) reset the trace file between runs if desired

    * `cyclelog`



1) Cleanup Steps:

    * `sudo rmmod buffer_python_client`

    * `sudo rmmod active_buffer_python_server`

    * `cd ~/software_defined_customization/layer4_5_modules/sample_modules`

    * `make clean`

  


1) Copy tracelog to file and refresh:

    * `tracecopy FILENAME`

    * copies tracelog to ~/software_defined_customization folder
