---
title: Home
layout: default
nav_order: 1
---
# Software Defined Network Customization at Layer 4.5


Prototype of Layer 4.5 customization framework to match NetSoft 2022 paper titled ["Towards Software Defined Layer 4.5 Customization"].  

Layer 4.5 contains a Network-wide Customization Orchestrator (NCO) to distribute Layer 4.5 customization modules to devices.  The NCO communicates with a Device Customization Agent (DCA) to deliver the module (DCA\_user).  The DCA\_kernel code will handle the registration of the customization module and inserting the module into the socket flow between the socket layer and transport layer.

["Towards Software Defined Layer 4.5 Customization"]: https://ieeexplore.ieee.org/abstract/document/9844096
