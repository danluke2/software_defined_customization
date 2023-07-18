# Layer 4.5 TODO's and Questions


# ADMIN
1) Migrate this file in its entirety to the Jira Kanban Board, on the TRMC where this repo is hosted. 

1) Add a licence to this project (Discussed in plan for course)

# TODO:


1) general module to do lots of functionality (like dialecting module);
data collection, dialecting, maybe encryption

    * allow more module parameters to be set at compile time (construction)

    * How to deal with IP address changes? Parameters to modules could set at load time if
    necessary (or update in real time?).


1) NCO signed modules: https://ubuntu.com/blog/how-to-sign-things-for-secure-boot


1) Expand DCA reports to NCO (ex: module 5-tuples)


1) NCO middlebox API and associated processing


# Questions/Possible TODO's:

1) Could I tag encrypted traffic with app/host ID to allow storing data without
decrypting it?  (searchable encryption to some degree)


1) Introduce IP mask to match subnets instead of just exact IP's?  

1) Can a module be applied to a list of IP's/ports/applications/etc?

1) better hash key for customization sockets?


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
