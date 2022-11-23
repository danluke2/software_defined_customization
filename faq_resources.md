# Layer 4.5 FAQ

# Purpose:
Common items I refer to when trying to understand code and functions in the kernel.

### Stuff:

1) C/Kernel coding standards: seem to be a lot of them

    https://www.kernel.org/doc/html/latest/process/coding-style.html#printing-kernel-messages

    https://codingart.readthedocs.io/en/latest/c/Naming.html

1) kernel modules:

    * https://sysprog21.github.io/lkmpg/

    * kernel module parameters:  https://tldp.org/LDP/lkmpg/2.6/html/x323.html

    * module basics:  https://lkw.readthedocs.io/en/latest/doc/04_exporting_symbols.html#one-module-dependent-on-several-modules

    * make file: https://tldp.org/LDP/lkmpg/2.4/html/x208.html

1) How sockets work in c and other guides:

    * https://beej.us/guide/



1) KBUILD_EXTRA_SYMBOLS

    this allows the module to use exported functions from another module not
    in the same tree.

    http://www.kernel.org/doc/Documentation/kbuild/modules.txt

    https://www.kernel.org/doc/Documentation/kbuild/modules.txt

    http://embeddedguruji.blogspot.com/2019/04/kbuildextrasymbols-using-symbols.html

1) kernel hash tables:

    * bitsize is a power-of-2 size -> number of buckets

    * since this holds all socket connections, we should have a decent number of buckets.

    * somewhere around 10 should be plenty to minimize collisions

    https://stackoverflow.com/questions/60870788/how-to-use-the-kernel-hashtable-api

    https://lwn.net/Articles/510202/


1) finding a kernel function and how it works:

    https://elixir.bootlin.com/linux/latest/source/include/linux/hashtable.h#L157

1) iov or kvec:

    https://lwn.net/Articles/625077/

    https://www.spinics.net/lists/linux-fsdevel/msg196508.html

1) how kernel sockets work

    http://vger.kernel.org/~davem/skb_data.html


    https://ops.tips/blog/how-linux-creates-sockets/


1) static functions:

    https://stackoverflow.com/questions/14423333/static-functions-in-linux-device-driver

1) function pointers in structs, args, arrays:

    https://stackoverflow.com/questions/1350376/function-pointer-as-a-member-of-a-c-struct

    https://stackoverflow.com/questions/1789807/function-pointer-as-an-argument

    http://venkateshabbarapu.blogspot.com/2012/09/function-pointers-and-callbacks-in-c.html

    https://stackoverflow.com/questions/252748/how-can-i-use-an-array-of-function-pointers


1) Why not read a config file from kernel:

    https://www.linuxjournal.com/article/8110



1) socket buffer allocations:

    https://access.redhat.com/discussions/3624151


1) trace file:

    https://www.kernel.org/doc/Documentation/trace/ftrace.txt


1) VBOX shared folder permissions:

    https://stackoverflow.com/questions/26740113/virtualbox-shared-folder-permissions



1) apache2 server with ssl:

   https://www.digitalocean.com/community/tutorials/how-to-create-a-self-signed-ssl-certificate-for-apache-in-ubuntu-20-04

   https://ubuntu.com/server/docs/web-servers-apache


1) nginx server with ssl:

    https://www.digitalocean.com/community/tutorials/how-to-install-nginx-on-ubuntu-18-04
    
    https://www.digitalocean.com/community/tutorials/how-to-create-a-self-signed-ssl-certificate-for-nginx-in-ubuntu-18-04

    https://www.digitalocean.com/community/tutorials/how-to-secure-nginx-with-let-s-encrypt-on-ubuntu-20-04


1) prevent ubuntu updates:

    https://askubuntu.com/questions/678630/how-can-i-avoid-kernel-updates/678633#678633

    https://askubuntu.com/questions/938494/how-to-i-prevent-ubuntu-from-kernel-version-upgrade-and-notification


1) get task from tgid:

   https://linux-kernel.vger.kernel.narkive.com/oJRnbfkU/find-task-by-pid-problem


1) python multiprocessing/logging:

    https://fanchenbao.medium.com/python3-logging-with-multiprocessing-f51f460b8778

1) VBox on mac display issues:

    https://apple.stackexchange.com/questions/429673/low-resolution-mode-in-macos-monterey

    https://www.reddit.com/r/virtualbox/comments/houi9k/how_to_fix_virtualbox_61_running_slow_on_mac/

    
1) Monitoring and tuning the network stack:

    https://blog.packagecloud.io/monitoring-tuning-linux-networking-stack-receiving-data/#ip-protocol-layer


1) Direct action mode for eBPF:

    https://qmonnet.github.io/whirl-offload/2020/04/11/tc-bpf-direct-action/



1) VSCODE Linux Headers:

    https://stackoverflow.com/questions/47866088/include-linux-kernel-headers-for-intellisense-in-vs-code

    