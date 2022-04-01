#!/bin/bash

# $1 = number of eth interfaces
# $2 = bridge ip address
# $3 = controller ip address

# Usual directory for downloading software in ProtoGENI hosts is `/local`
cd /local

SWITCHNAME="br0"
IFINFO=InterfacesInfo


##### Check if file is there #####
if [ ! -f "./InterfacesInfo" ]
then
    #### Create the file ####
    sudo touch "./InterfacesInfo"

    # Save Interface Infromation
    ifconfig -a > $IFINFO

    # Create the switch if it doesn't already exist
    sudo ovs-vsctl add-br $SWITCHNAME


    # Add ports to bridge for each of the interfaces and clear IP addresses
    for i in $(seq 1 $1)
  	do
      sudo ovs-vsctl add-port $SWITCHNAME eth${i}
      sudo ifconfig eth${i} 0
    done

    sudo ifconfig $SWITCHNAME $2/24 up
    sudo ovs-vsctl set-controller $SWITCHNAME tcp:$3:6633
    sudo ovs-vsctl set-fail-mode $SWITCHNAME standalone

fi

#rm -f $IFTMPFILE
