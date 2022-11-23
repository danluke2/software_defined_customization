#!/bin/bash

#Purpose:  establishes DCA service on a Layer 4.5 host or middlebox

# ************** STANDARD PARAMS MUST GO HERE ****************
DCA_LOCATION=/usr/lib/modules/5.13.0-35-generic/layer4_5/DCA
# ************** END STANDARD PARAMS ****************

# Force root
if [[ "$(id -u)" != "0" ]]; then
	echo "This script must be run as root" 1>&2
	exit -1
fi

echo "Installing DCA service"

LOADER_FILE=/etc/systemd/system/dca.service

if test -f "$LOADER_FILE"; then
	echo "$LOADER_FILE exists."
	rm $LOADER_FILE
fi

touch $LOADER_FILE

cat <<EOT >>$LOADER_FILE
[Unit]
Description=DCA systemd service.

[Service]
Type=simple
ExecStart='$DCA_LOCATION/DCA.py'

[Install]
WantedBy=multi-user.target
EOT

#register loader with systemd
systemctl enable dca.service

# start DCA service
systemctl start dca.service
