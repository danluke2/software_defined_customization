#!/bin/bash
set -e

# Init
PM="/bin/apt" # Package Manager
CURDIR="$( pwd )"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )" #directory of make file

# Force root
if [[ "$(id -u)" != "0" ]];
then
	echo "This script must be run as root" 1>&2
	exit -1
fi


# ----------------------------------------------------------------
# Function for exit due to fatal program error
#   Accepts 1 argument:
#     string containing descriptive error message
# ----------------------------------------------------------------
error_exit()
{
  echo "${PROGNAME}: ${1:-"Unknown Error"}" 1>&2
  exit 1
}


echo "***************************"
echo "Installing dependencies"
# Required build libraries
$PM install python3 python3-pip linux-headers-$(uname -r)

# libconfig-dev libcap-dev python-dev libevent-dev

#for python encryption support in DCA, uncomment if needed
pip install pycryptodome


INSTALL_LOCATION=/usr/lib/modules/$(uname -r)/layer4_5

echo "***************************"
echo "Making Layer 4.5 modules"

# Make the files
cd $DIR
echo $DIR
make && make install || error_exit "Makefile error detected"



# Start layer 4.5 processing
insmod $INSTALL_LOCATION/layer4_5.ko layer4_5_path="$INSTALL_LOCATION"
echo "Layer 4.5 started"


echo "***************************"
echo "Adding Layer 4.5 to modules config for auto load on boot"
MODULE_FILE=/etc/modules-load.d/modules.conf
if grep -Fxq "layer4_5" $MODULE_FILE
then
  echo "Layer 4.5 already added to file"
else
	echo "layer4_5" >> $MODULE_FILE
fi


# if layer4_5 module insertion succeeds, then build the loader service and enable
echo "***************************"
echo "Installing loader service"
cp bash/loader.sh $INSTALL_LOCATION/
mkdir -p $INSTALL_LOCATION/customizations

cd $CURDIR


LOADER_FILE=/etc/systemd/system/layer4_5_loader.service
#create systemd service for the loader
#set -o noclobber

if test -f "$LOADER_FILE"; then
    echo "$LOADER_FILE exists."
		rm $LOADER_FILE
fi

touch $LOADER_FILE

cat <<EOT >> $LOADER_FILE
[Unit]
Description=Customization loader systemd service.

[Service]
Type=simple
ExecStart='/usr/lib/modules/$(uname -r)/layer4_5/loader.sh'

[Install]
WantedBy=multi-user.target
EOT

#register loader with systemd
systemctl enable layer4_5_loader.service


echo "***************************"
echo "(TODO) Installing DCA service"
