#!/bin/bash

#Purpose:  establishes Python simple server service





# ************** STANDARD PARAMS MUST GO HERE ****************
SIMPLE_SERVER_DIR=/home/vagrant/software_defined_customization/experiment_scripts/client_server




# ************** STANDARD PARAMS MUST GO HERE ****************

# Force root
if [[ "$(id -u)" != "0" ]];
then
	echo "This script must be run as root" 1>&2
	exit -1
fi



echo "Installing Python Simple Server service"


LOADER_FILE=/etc/systemd/system/simple_server.service

if test -f "$LOADER_FILE"; then
    echo "$LOADER_FILE exists."
		rm $LOADER_FILE
fi

touch $LOADER_FILE

cat <<EOT >> $LOADER_FILE
[Unit]
Description=Python simple server loader systemd service.

[Service]
Type=simple
ExecStart='$SIMPLE_SERVER_DIR/python_simple_server.py'

[Install]
WantedBy=multi-user.target
EOT

#register loader with systemd
systemctl enable simple_server.service

# start DCA service
systemctl start simple_server.service
