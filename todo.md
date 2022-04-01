# Layer 4.5 TODO's and Questions


# TODO:

1) Further test/improve overhead of using LKM approach

1) how to reset cust module global variables or make socket specific (i.e., one cust mod applies to 2 sockets, then both update the same global vars)

    * providing socket flow params can help distinguish different flows


1) general module to do lots of functionality (like dialecting module);
data collection, dialecting, maybe encryption

    * allow more module parameters to be set at compile time (construction)

    * How to deal with IP address changes? Parameters to modules could set at load time if
    necessary (or update in real time?).


1) NCO signed modules: https://ubuntu.com/blog/how-to-sign-things-for-secure-boot


1) Expand DCA reports to NCO (ex: module 5-tuples)


1) NCO middlebox API and associated processing



1) expand support for TLS traffic:

    * this applies to the recieve side only since that may be breaking

    * could we allow a receive loop between DCA and module to ensure all
    encrypted traffic needed by app is delivered?

    * could we be seeing problems from multiple iter buff segments?

1) Could I tag encrypted traffic with app/host ID to allow storing data without
decrypting it?  (searchable encryption to some degree)


traffic translation and middlebox traversal (same design framework)

# Questions:

1) Should we change recv logic: cust lookup first, if present put the L4 buffer
into the cust buffer, then cust module modifies buffer as needed and L4.5 puts
desired bytes into app buffer.  Downside of intermediary buffer could
be that TCP closes connection b/c all data received by app from L4 point of view.

1) Introduce IP mask to match subnets instead of just exact IP's?  

1) Can a module be applied to a list of IP's/ports/applications/etc?

1) better hash key for customization sockets?

1) When closing a socket, do we care if it was found or just that it is not
present anymore?  Leaning to just not present.

1) tap into UDP init also?  Not sure super useful


1) does/can malware/worms alias as legitimate application? What about root kit?
what about establishing a backdoor?  Mitre attack website to see types of threats
and methods used to achieve goals.


1) If network messages included the application tag and what process ID generated
the traffic, could you detect malicious apps?  Thought is that a malicious app
may use more threads under the same socket than the standard app does.  Also,
malicious app trying to piggy-back on legitimate app might have process ID in
a different range than legitimate app.

1) What other info would be good to aid ML on network traffic and help create
the training class?

1) check the number of segments in send/recv b/c if more than 1, that may be a problem?


# TODO (If Kernel Mod Desired):

1) Transition TCP and UDP wrappers into socket wrappers so no L4 distinction needed.

    * inet_sendmsg and inet_recvmsg would be the new tap points since those call
      udp or tcp functions.  Maybe introduce BPF here also as socket cust option.


    * Socket creation can do preliminary inquiry to match application/protocol
      to inform inet tap.  If app/proto not customized, then inet can call udp or
      tcp directly.  Else, we check at inet and set customization if necessary.

    * socket can hold parameter for customization: allow quick skip and only
      track customized sockets; thus we introduce an 'if' check to paths, which
      should be negligible for non-custom sockets


1) Can we transition to BPF functionality instead (multiple TODO for this)

    * Expand L3AF project to support BPF_SOCK_CUST programs also

    * Can BPF tap UDP and attach customization?
