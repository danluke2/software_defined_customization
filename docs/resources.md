---
layout: post
title: Useful References
nav_order: 5
---
# Useful References
{: .no_toc}
> Common items I refer to when trying to understand code and functions in the kernel.

<details open markdown="block">
  <summary>
    Table of contents
  </summary>
  {: .text-delta }
1. TOC
{:toc}
</details>


## Papers

 - [Software Defined Customization of Network Protocols with LAYER 4.5](https://calhoun.nps.edu/handle/10945/71078){:target="_blank"}

 - [Dissertation Defense](https://github.com/danluke2/software_defined_customization/blob/main/Dissertation%20Papers%20and%20Videos/Dissertation_Defense.pdf){:target="_blank"}

 - [Towards Software Defined Customization (NetSoft 22 Paper)](https://ieeexplore.ieee.org/abstract/document/9844096){:target="_blank"}

 - [Demo: Towards Software Defined Customization (NetSoft 22 Demo Paper)](https://ieeexplore.ieee.org/abstract/document/9844104){:target="_blank"}

 - [Towards Agile Network Operation with Layer 4.5 Protocol Customization](https://github.com/danluke2/software_defined_customization/blob/main/Dissertation%20Papers%20and%20Videos/NetVerify_2021_Paper.pdf){:target="_blank"}

## Videos

 - [NetVerify 21 presentation focused on Layer 4.5 and initial idea of network wide control](https://youtu.be/s9vwJLDMSlI?start=17737&end=18730){:target="_blank"}
  
 - [NetSoft 22 Demo](https://whova.com/portal/ieeen_202206/videos/3IjMyUTN4QTO/){:target="_blank"}
  
 - [NetSoft Presentation](https://whova.com/portal/ieeen_202206/videos/3IjM0ATOxIDN/){:target="_blank"}

## External References

### C/Kernel coding standards

 - [Linux Kernel style guide](https://www.kernel.org/doc/html/latest/process/coding-style.html#){:target="_blank"}

 - [~~coding art Naming guide~~](https://codingart.readthedocs.io/en/latest/c/Naming.html){:target="_blank"} (link broken)

### Kernel Modules

 - [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/){:target="_blank"}

 - [Kernel Module parameters](https://tldp.org/LDP/lkmpg/2.6/html/x323.html){:target="_blank"}

 - [Module Basics](https://lkw.readthedocs.io/en/latest/doc/04_exporting_symbols.html#one-module-dependent-on-several-modules){:target="_blank"}

 - [Make File](https://tldp.org/LDP/lkmpg/2.4/html/x208.html){:target="_blank"}

### How sockets work in C and other guides

 - [Beej's Guides](https://beej.us/guide/){:target="_blank"}


### KBUILD_EXTRA_SYMBOLS

 - This allows the module to use exported functions from another module not
    in the same tree.

 - [Building External Modules](https://www.kernel.org/doc/Documentation/kbuild/modules.txt){:target="_blank"}

 - [Using Symbols Exported from External Modules](http://embeddedguruji.blogspot.com/2019/04/kbuildextrasymbols-using-symbols.html){:target="_blank"}

### Kernel Hash Tables

 -  bitsize is a power-of-2 size -> number of buckets

 -  since this holds all socket connections, we should have a decent number of buckets.

 -  somewhere around 10 should be plenty to minimize collisions

 - [Kernel Hash Tables](https://stackoverflow.com/questions/60870788/how-to-use-the-kernel-hashtable-api){:target="_blank"}

 - [Hash Table Overview](https://lwn.net/Articles/510202/){:target="_blank"}


### Finding a Kernel Function

 - [Linux hashtable.h](https://elixir.bootlin.com/linux/latest/source/include/linux/hashtable.h#L157){:target="_blank"}

### iov or kvec

 - [The iov_iter Interface](https://lwn.net/Articles/625077/){:target="_blank"}

 - [iov_iter Design Discussion](https://www.spinics.net/lists/linux-fsdevel/msg196508.html){:target="_blank"}

### How Kernel Sockets Work

 - [Layout of SKB data](http://vger.kernel.org/~davem/skb_data.html){:target="_blank"}

 - [How Linux Creates Sockets](https://ops.tips/blog/how-linux-creates-sockets/){:target="_blank"}


### Functions in Linux and C

 - [Static functions in Linux device drivers](https://stackoverflow.com/questions/14423333/static-functions-in-linux-device-driver){:target="_blank"}

 - [Function pointer as a member of a C struct](https://stackoverflow.com/questions/1350376/function-pointer-as-a-member-of-a-c-struct){:target="_blank"}

 - [Function pointer as an argument](https://stackoverflow.com/questions/1789807/function-pointer-as-an-argument){:target="_blank"}

 - [Function Pointers and Callbacks in C](http://venkateshabbarapu.blogspot.com/2012/09/function-pointers-and-callbacks-in-c.html){:target="_blank"}
